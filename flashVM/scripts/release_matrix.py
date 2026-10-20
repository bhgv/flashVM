#!/usr/bin/env python3
"""Audit portable native-report bundles without claiming full roadmap acceptance.

A bundle is platform-evidence.json plus each profile's test-inventory.log,
test-inventory-after.log and ctest-results.xml. Paths stored by another machine
are never executed or followed. Missing platforms remain blocked, never skipped.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile
from ctest_evidence import inventory, read_bounded, results
from native_target import architecture


PROFILES = ('agg', 'cairo', 'minimal', 'minimal-jit')
SYSTEMS = ('Linux', 'Windows', 'Darwin', 'Android')
PHASES = ('configure', 'build', 'test-inventory', 'test', 'test-inventory-after')
DEFAULT_TARGETS = tuple((system, machine, profile)
                        for system, machine in [('Linux', 'x86_64'), ('Windows', 'x86_64'),
                                                 ('Darwin', 'aarch64'), ('Android', 'aarch64')]
                        for profile in PROFILES)


def cell(value):
    """Parse an explicit OS:architecture:profile; no shell or wildcard expansion."""
    parts = value.split(':')
    if len(parts) != 3 or parts[0] not in SYSTEMS or parts[2] not in PROFILES:
        raise ValueError('expected OS:architecture:profile')
    return parts[0], architecture(parts[1]), parts[2]


def skip_rule(value):
    """Keep exact test names, even when they contain a colon."""
    parts = value.split(':', 3)
    if len(parts) != 4 or not parts[3]:
        raise ValueError('expected OS:architecture:profile:exact-test-name')
    return cell(':'.join(parts[:3])), parts[3]


def identity(value, commit):
    """Validate stable component-source identity; OS-specific hashes may differ."""
    return (isinstance(value, dict) and value.get('commit') == commit
            and isinstance(value.get('sha256'), str)
            and re.fullmatch(r'[0-9a-f]{64}', value['sha256']) is not None
            and type(value.get('files')) is int and value['files'] > 0)


def timestamp(value):
    """Require explicit UTC offsets instead of locale-dependent naive time."""
    if not isinstance(value, str):
        raise ValueError('missing report timestamp')
    result = datetime.datetime.fromisoformat(value.replace('Z', '+00:00'))
    if result.tzinfo is None or result.utcoffset() is None:
        raise ValueError('report timestamp has no timezone')
    return result


def artifact(bundle, profile, name):
    """Confine portable artifacts to the supplied bundle, including symlinks."""
    path = (bundle / profile / name).resolve()
    try:
        path.relative_to(bundle)
    except ValueError as error:
        raise ValueError('report artifact escapes bundle') from error
    return path


def report_cells(path, commit, version, acknowledgements):
    """Verify a schema-2 native report and recompute its exact CTest evidence."""
    if not re.fullmatch(r"(?:[0-9a-f]{40}|[0-9a-f]{64})", commit):
        raise ValueError("commit must be a full Git object ID")
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+)+", version):
        raise ValueError("invalid release version")
    path = Path(path).resolve()
    data = read_bounded(path)
    report = json.loads(data)
    if not isinstance(report, dict) or report.get('schema') != 2 or report.get('status') != 'passed':
        raise ValueError('requires a passed schema-2 native report')
    if report.get('commit') != commit or report.get('version') != version or report.get('dirty') is not False:
        raise ValueError('report commit/version differs or worktree was not clean')
    if (not identity(report.get('source_start'), commit)
            or report.get('source_start') != report.get('source_end')):
        raise ValueError('source endpoints are missing or differ')
    if timestamp(report.get('finished_utc')) < timestamp(report.get('started_utc')):
        raise ValueError('report finished before it started')
    host = report.get('host')
    profiles = report.get('profiles')
    if not isinstance(host, dict) or not isinstance(profiles, list) or not profiles:
        raise ValueError('missing host or profiles')
    if host.get('system') not in ('Linux', 'Windows', 'Darwin'):
        raise ValueError('desktop report cannot certify Android device execution')
    machine = architecture(host.get('machine'))
    seen, accepted = set(), []
    for profile in profiles:
        if not isinstance(profile, dict) or profile.get('name') not in PROFILES:
            raise ValueError('invalid profile name')
        name = profile['name']
        if name in seen or profile.get('status') != 'passed':
            raise ValueError('duplicate or failed profile')
        seen.add(name)
        target = profile.get('native_target')
        width = 8 if machine in ('x86_64', 'aarch64', 'riscv64') else 4
        if (not isinstance(target, dict) or target.get('system') != host['system']
                or target.get('machine') != machine or type(target.get('pointer_size')) is not int
                or target['pointer_size'] != width):
            raise ValueError('native target and reporter do not agree')
        compilers = profile.get('toolchains')
        if not isinstance(compilers, dict):
            raise ValueError('missing compiler provenance')
        for language in ('C', 'CXX'):
            compiler = compilers.get(language)
            if (not isinstance(compiler, dict) or not compiler.get('id')
                    or not isinstance(compiler.get('version'), str)
                    or not re.fullmatch(r'[0-9]+(?:\.[0-9]+)*', compiler['version'])
                    or type(compiler.get('pointer_size')) is not int or compiler['pointer_size'] != width):
                raise ValueError('incomplete compiler identity or ABI')
            if compiler.get('architecture') and architecture(compiler['architecture']) != machine:
                raise ValueError('compiler architecture differs from report target')
        commands = profile.get('commands')
        if not isinstance(commands, list) or not commands:
            raise ValueError('missing validation commands')
        phases = []
        for command in commands:
            if (not isinstance(command, dict) or command.get('status') != 'passed'
                    or type(command.get('returncode')) is not int or command['returncode'] != 0
                    or not isinstance(command.get('argv'), list) or not command['argv']
                    or any(not isinstance(arg, str) or not arg for arg in command['argv'])):
                raise ValueError('invalid or unsuccessful command provenance')
            phases.append(command.get('phase'))
        if phases[:len(PHASES)] != list(PHASES) or len(set(phases)) != len(phases):
            raise ValueError('missing, reordered or duplicate validation phases')
        key = host['system'], machine, name
        # Use relocated sibling artifacts, never absolute paths from recorded argv.
        expected = inventory(artifact(path.parent, name, 'test-inventory.log'))
        after = inventory(artifact(path.parent, name, 'test-inventory-after.log'))
        if expected != after:
            raise ValueError('test definitions changed during execution')
        actual = results(artifact(path.parent, name, 'ctest-results.xml'), expected,
                         acknowledgements.get(key, ()))
        if profile.get('tests') != actual:
            raise ValueError('report summary does not match CTest artifacts')
        accepted.append((key, {'report': str(path), 'report_sha256': hashlib.sha256(data).hexdigest(),
                               'source_sha256': report['source_start']['sha256'], 'tests': actual}))
    return accepted


def evaluate(paths, commit, version, required=DEFAULT_TARGETS, acknowledgements=None):
    """Build a deterministic matrix; absence blocks and contradictory input fails."""
    if not re.fullmatch(r'(?:[0-9a-f]{40}|[0-9a-f]{64})', commit):
        raise ValueError('commit must be a full Git object ID')
    if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)+', version):
        raise ValueError('invalid release version')
    required = list(required)
    if not required or len(set(required)) != len(required):
        raise ValueError('required cells must be nonempty and unique')
    known = {}
    for path in paths:
        for key, value in report_cells(path, commit, version, acknowledgements or {}):
            if key in known:
                raise ValueError('duplicate evidence for ' + ':'.join(key))
            known[key] = value
    cells = []
    for key in sorted(required):
        value = {'system': key[0], 'machine': key[1], 'profile': key[2],
                 'status': 'passed' if key in known else 'missing'}
        if key in known:
            value.update(known[key])
        cells.append(value)
    return {'schema': 1, 'commit': commit, 'version': version,
            'scope': 'native execution matrix only; not complete roadmap/SWF/device/static-link acceptance',
            'status': 'passed' if all(c['status'] == 'passed' for c in cells) else 'blocked',
            'cells': cells, 'extra_cells': [':'.join(key) for key in sorted(set(known) - set(required))]}


def write_report(path, report):
    """Atomically publish complete JSON; readers never see a partial passing file."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent,
                                         prefix='.' + path.name + '.', delete=False) as stream:
            temporary = Path(stream.name)
            json.dump(report, stream, indent=2)
            stream.write('\n')
        os.replace(str(temporary), str(path))
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    """Report blocked/invalid runs with nonzero status, replacing an earlier pass."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', action='append', default=[], type=Path)
    parser.add_argument('--commit', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--require', action='append', default=[], metavar='OS:ARCH:PROFILE')
    parser.add_argument('--allow-skip', action='append', default=[], metavar='OS:ARCH:PROFILE:TEST')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    # Never overwrite a supplied bundle, even before discovering its profiles.
    for path in args.report:
        bundle = path.resolve().parent
        if output == path.resolve() or output.is_relative_to(bundle):
            parser.error('output must be outside every input bundle')
    result = {'schema': 1, 'status': 'invalid', 'commit': args.commit, 'version': args.version}
    try:
        rules = {}
        for value in args.allow_skip:
            key, name = skip_rule(value)
            rules.setdefault(key, []).append(name)
        result = evaluate(args.report, args.commit, args.version,
                          [cell(value) for value in args.require] if args.require else DEFAULT_TARGETS, rules)
        code = 0 if result['status'] == 'passed' else 1
    except (OSError, ValueError, TypeError, KeyError) as error:
        result['error'] = str(error)
        code = 2
    write_report(output, result)
    print(result['status'] + ': ' + str(output))
    return code


if __name__ == '__main__':
    sys.exit(main())
