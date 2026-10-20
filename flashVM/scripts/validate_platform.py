#!/usr/bin/env python3
"""Configure, build and execute native release profiles, recording actual evidence.

Requires CMake/CTest 3.26+ and a native compiler. This runner deliberately rejects
cross-builds; Android uses android_test.py and CTest on its device instead.
Reports/logs stay in the ignored build directory and never certify roadmap stages.
"""
import argparse
import datetime
import json
import hashlib
import stat
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import tempfile
from elf_dependencies import validate_elf_dependencies
from ctest_evidence import inventory, results


def source_identity(root):
    """Fingerprint Git-listed component inputs without following external symlinks.

    Ignored build products are excluded. Missing tracked files remain inputs;
    staged content and HEAD are included so an index-only edit is also detected.
    This is an endpoint check, not a lock or an immutable build sandbox.
    """
    def git(*args):
        """Read raw NUL-delimited Git names safely, including non-UTF8 names."""
        return subprocess.check_output(['git', '-C', str(root)] + list(args))

    digest = hashlib.sha256()

    def feed(value):
        """Length-prefix fields to prevent ambiguous concatenated records."""
        digest.update(str(len(value)).encode('ascii') + b':' + value)

    commit = git('rev-parse', 'HEAD').decode('ascii').strip()
    feed(commit.encode('ascii'))
    feed(git('ls-files', '--stage', '-z', '--', '.'))
    names = sorted(set(git('ls-files', '--cached', '--others', '--exclude-standard',
                           '-z', '--', '.').split(b'\0')) - {b''})
    for name in names:
        feed(name)
        path = root / os.fsdecode(name)
        try:
            mode = path.lstat().st_mode
        except FileNotFoundError:
            feed(b'missing')
            continue
        if stat.S_ISLNK(mode):
            feed(b'link')
            feed(os.fsencode(os.readlink(str(path))))
        elif stat.S_ISREG(mode):
            feed(b'executable' if mode & stat.S_IXUSR else b'file')
            content = hashlib.sha256()
            with path.open('rb') as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    content.update(block)
            feed(content.digest())
        else:
            raise ValueError('unsupported source input (including submodules): ' + os.fsdecode(name))
    return {'commit': commit, 'sha256': digest.hexdigest(), 'files': len(names)}


def verify_source_identity(root, report):
    """Prevent a passing report when source endpoints differ or cannot be read."""
    report['source_end'] = source_identity(root)
    if report['source_start'] != report['source_end']:
        raise ValueError('source inputs changed during platform validation; rerun on stable sources')


def cache_values(path):
    """Read typed CMake cache entries without interpreting paths as shell code."""
    values = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        if line and not line.startswith(('#', '//')) and '=' in line:
            key, value = line.split('=', 1)
            values[key.split(':', 1)[0]] = value
    return values


def compiler_identities(build, cache):
    """Record active CMake compiler detection without executing generated CMake code."""
    parts = [cache.get('CMAKE_CACHE_' + part + '_VERSION', '')
             for part in ('MAJOR', 'MINOR', 'PATCH')]
    if any(not re.fullmatch(r'[0-9]+', part) for part in parts):
        raise ValueError('active CMake cache version is missing or malformed')
    directory = build / 'CMakeFiles' / '.'.join(parts)
    identities = {}
    fields = {'id': 'COMPILER_ID', 'version': 'COMPILER_VERSION',
              'platform': 'PLATFORM_ID', 'architecture': 'COMPILER_ARCHITECTURE_ID',
              'frontend': 'COMPILER_FRONTEND_VARIANT', 'target': 'COMPILER_TARGET',
              'simulate_id': 'SIMULATE_ID', 'simulate_version': 'SIMULATE_VERSION',
              'pointer_size': 'SIZEOF_DATA_PTR'}
    for language in ('C', 'CXX'):
        path = directory / ('CMake' + language + 'Compiler.cmake')
        values = {}
        for line in path.read_text(encoding='utf-8').splitlines():
            match = re.fullmatch(r'set\((CMAKE_[A-Z0-9_]+) "([^"\r\n]*)"\)', line)
            if match:
                values[match[1]] = match[2]
        identity = {name: values.get('CMAKE_' + language + '_' + suffix, '')
                    for name, suffix in fields.items()}
        if (not identity['id'] or not re.fullmatch(r'[0-9]+(?:\.[0-9]+)*', identity['version'])
                or not re.fullmatch(r'[1-9][0-9]*', identity['pointer_size'])):
            raise ValueError(language + ' compiler identity/version/ABI evidence is incomplete')
        identity['pointer_size'] = int(identity['pointer_size'])
        identity['metadata'] = str(path)
        identities[language] = identity
    if identities['C']['pointer_size'] != identities['CXX']['pointer_size']:
        raise ValueError('C and C++ compiler pointer sizes differ')
    return identities


def tool_version(executable):
    """Reject old tools before relying on modern CTest failure/selection options."""
    output = subprocess.check_output([executable, '--version'], text=True, stderr=subprocess.STDOUT)
    first = output.splitlines()[0] if output.splitlines() else ''
    match = re.search(r'\bversion (\d+)\.(\d+)\.(\d+)', first)
    if not match or tuple(int(value) for value in match.groups()) < (3, 26, 0):
        raise RuntimeError(executable + ': CMake/CTest 3.26+ required; got ' + first)
    return first


def encoder_command(document):
    """Resolve the configured backend fixture test from authoritative CTest JSON."""
    if not isinstance(document, dict) or not isinstance(document.get('version'), dict):
        raise ValueError('invalid CTest JSON document')
    if document['version'].get('major') != 1 or not isinstance(document.get('tests'), list):
        raise ValueError('unsupported CTest JSON schema')
    tests = document['tests']
    if any(not isinstance(test, dict) for test in tests):
        raise ValueError('invalid CTest test entry')
    selected = [test for test in tests if test.get('name') == 'flashvm_tests_jit_backends']
    if len(selected) != 1:
        raise ValueError('exactly one backend fixture test is required')
    command = selected[0].get('command')
    if not isinstance(command, list) or not command or any(
            not isinstance(part, str) or not part for part in command):
        raise ValueError('backend fixture command is unavailable or malformed')
    return command[:]


def validate_encoder(root, build, ctest, execute, record):
    """Fail closed on emulator/discovery/generation errors; never reuse stale fixtures."""
    evidence = {'status': 'failed', 'scope': 'synthetic ISA/ABI with stub helpers; not a foreign VM'}
    record['jit_encoder'] = evidence
    execute('jit-discover', [ctest, '--test-dir', str(build), '-C', 'Release',
                             '--show-only=json-v1'])
    command = encoder_command(json.loads((build / 'jit-discover.log').read_text(encoding='utf-8')))
    execute('jit-emulator-version', [sys.executable, '-c',
                                     'import unicorn; print(unicorn.__version__)'])
    version = (build / 'jit-emulator-version.log').read_text(encoding='utf-8').strip()
    if not version:
        raise ValueError('emulator version was not reported')
    evidence['unicorn_version'] = version
    fixtures = Path(tempfile.mkdtemp(prefix='jit-encoder-', dir=str(build)))
    evidence['fixtures'] = str(fixtures)
    execute('jit-fixtures', command + [str(fixtures)])
    execute('jit-emulation', [sys.executable, str(root / 'scripts/check_jit_backends.py'),
                               str(fixtures)])
    evidence['status'] = 'passed'


def main():
    """Run selected profiles and always leave a pass/fail record, even on interruption."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['agg', 'cairo', 'minimal', 'minimal-jit', 'all'], default='all')
    parser.add_argument('--build-root', type=Path)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--ctest', default='ctest')
    parser.add_argument('--generator')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--jit-emulator', action='store_true',
                        help='require optional Unicorn backend instruction/ABI execution')
    parser.add_argument('--elf-dependencies', action='store_true',
                        help='audit direct Linux ELF dependencies of CTest artifacts')
    parser.add_argument('--readelf', default='readelf', help='GNU readelf executable for ELF audit')
    parser.add_argument('--allow-skip', action='append', default=[], metavar='TEST_NAME',
                        help='acknowledge one exact skipped test name; never counts as passed')
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    root = Path(__file__).resolve().parents[1]
    output = (args.build_root or root / 'build-platform').resolve()
    output.mkdir(parents=True, exist_ok=True)
    report = {'schema': 2, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'host': {'system': platform.system(), 'machine': platform.machine(),
                       'release': platform.release(), 'python': platform.python_version()},
              'version': (root / 'VERSION.txt').read_text().strip(),
              'profiles': [], 'status': 'running'}
    # Replace any previous pass before launching tools. Even an uncatchable
    # process termination then leaves running evidence, never a stale pass.
    (output / 'platform-evidence.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    try:
        report['source_start'] = source_identity(root)
        report['commit'] = report['source_start']['commit']
        report['dirty'] = bool(subprocess.check_output(
            ['git', '-C', str(root), 'status', '--porcelain'], text=True).strip())
        report['tools'] = {name: tool_version(executable) for name, executable in
                           [('cmake', args.cmake), ('ctest', args.ctest)]}
        for name in (['agg', 'cairo', 'minimal', 'minimal-jit'] if args.profile == 'all' else [args.profile]):
            build = output / name
            build.mkdir(exist_ok=True)
            record = {'name': name, 'commands': [], 'status': 'failed'}
            report['profiles'].append(record)

            def execute(label, command):
                """Keep complete output per command; propagate any nonzero status."""
                print(name + ': ' + label, flush=True)
                entry = {'phase': label, 'argv': command, 'status': 'running'}
                if label.startswith('elf-'):
                    entry['environment'] = {'LC_ALL': 'C'}
                record['commands'].append(entry)
                try:
                    with (build / (label + '.log')).open('w', encoding='utf-8') as log:
                        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                                env=dict(os.environ, LC_ALL='C')
                                                if label.startswith('elf-') else None)
                except (OSError, KeyboardInterrupt) as error:
                    entry.update(status='failed', error=str(error))
                    raise
                entry.update(returncode=result.returncode,
                             status='passed' if result.returncode == 0 else 'failed')
                if result.returncode:
                    raise RuntimeError(name + ' ' + label + ' failed; see ' + str(build / (label + '.log')))

            command = [args.cmake, '-S', str(root), '-B', str(build),
                       '-DCMAKE_BUILD_TYPE=Release', '-DFVM_BUILD_TESTS=ON',
                       '-DFVM_BUILD_EXAMPLES=ON', '-DFVM_BUILD_REFERENCE_RUNNER=ON',
                       '-DFVM_DEFS_FILE=' + str(root / 'cmake/profiles' / (name + '.defs'))]
            if args.generator:
                command += ['-G', args.generator]
            execute('configure', command)
            cache = cache_values(build / 'CMakeCache.txt')
            # CMAKE_CROSSCOMPILING is not a cache variable. The generated system
            # description is authoritative even for an existing build directory.
            systems = list((build / 'CMakeFiles').glob('*/CMakeSystem.cmake'))
            if not systems or any('set(CMAKE_CROSSCOMPILING "TRUE")' in p.read_text() for p in systems):
                raise RuntimeError('native validation requires a native CMake target')
            record['compiler'] = {key: cache.get(key) for key in
                                  ['CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER', 'CMAKE_GENERATOR']}
            record['toolchains'] = compiler_identities(build, cache)
            execute('build', [args.cmake, '--build', str(build), '--config', 'Release',
                              '--parallel', str(args.jobs)])
            # Isolate CTest from inherited filters that could silently reduce evidence.
            if any(key in os.environ for key in ['CTEST_TESTS_TO_RUN', 'CTEST_TEST_LOAD']):
                raise RuntimeError('clear inherited CTest selection/load variables')
            execute('test-inventory', [args.ctest, '--test-dir', str(build), '-C', 'Release',
                                       '--show-only=json-v1'])
            expected = inventory(build / 'test-inventory.log')
            # Remove old success before execution, including reruns that fail to emit XML.
            junit = build / 'ctest-results.xml'
            junit.unlink(missing_ok=True)
            execute('test', [args.ctest, '--test-dir', str(build), '-C', 'Release',
                             '--output-on-failure', '--no-tests=error', '--parallel', str(args.jobs),
                             '--output-junit', str(junit)])
            record['tests'] = results(junit, expected, args.allow_skip)
            execute('test-inventory-after', [args.ctest, '--test-dir', str(build), '-C', 'Release',
                                             '--show-only=json-v1'])
            if inventory(build / 'test-inventory-after.log') != expected:
                raise ValueError('CTest definitions changed during execution')
            if args.jit_emulator:
                validate_encoder(root, build, args.ctest, execute, record)
            if args.elf_dependencies:
                validate_elf_dependencies(build, args.ctest, args.readelf, execute, record)
            record['status'] = 'passed'
        verify_source_identity(root, report)
        report['status'] = 'passed'
        return 0
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        report['status'] = 'failed'
        report['error'] = str(error)
        print(str(error), file=sys.stderr)
        return 1
    finally:
        if report['status'] == 'running':
            report['status'] = 'failed'
        report['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (output / 'platform-evidence.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    sys.exit(main())
