#!/usr/bin/env python3
"""Configure, build and execute native release profiles, recording actual evidence.

Requires CMake/CTest 3.26+ and a native compiler. This runner deliberately rejects
cross-builds; Android uses android_test.py and CTest on its device instead.
Reports/logs stay in the ignored build directory and never certify roadmap stages.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys


def cache_values(path):
    """Read typed CMake cache entries without interpreting paths as shell code."""
    values = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        if line and not line.startswith(('#', '//')) and '=' in line:
            key, value = line.split('=', 1)
            values[key.split(':', 1)[0]] = value
    return values


def tool_version(executable):
    """Reject old tools before relying on modern CTest failure/selection options."""
    output = subprocess.check_output([executable, '--version'], text=True, stderr=subprocess.STDOUT)
    first = output.splitlines()[0] if output.splitlines() else ''
    match = re.search(r'\bversion (\d+)\.(\d+)\.(\d+)', first)
    if not match or tuple(int(value) for value in match.groups()) < (3, 26, 0):
        raise RuntimeError(executable + ': CMake/CTest 3.26+ required; got ' + first)
    return first


def main():
    """Run selected profiles and always leave a pass/fail record, even on interruption."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['agg', 'cairo', 'minimal', 'all'], default='all')
    parser.add_argument('--build-root', type=Path)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--ctest', default='ctest')
    parser.add_argument('--generator')
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    root = Path(__file__).resolve().parents[1]
    output = (args.build_root or root / 'build-platform').resolve()
    output.mkdir(parents=True, exist_ok=True)
    report = {'schema': 1, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'host': {'system': platform.system(), 'machine': platform.machine(),
                       'release': platform.release(), 'python': platform.python_version()},
              'version': (root / 'VERSION.txt').read_text().strip(),
              'profiles': [], 'status': 'running'}
    # Replace any previous pass before launching tools. Even an uncatchable
    # process termination then leaves running evidence, never a stale pass.
    (output / 'platform-evidence.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    try:
        report['commit'] = subprocess.check_output(
            ['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
        report['dirty'] = bool(subprocess.check_output(
            ['git', '-C', str(root), 'status', '--porcelain'], text=True).strip())
        report['tools'] = {name: tool_version(executable) for name, executable in
                           [('cmake', args.cmake), ('ctest', args.ctest)]}
        for name in (['agg', 'cairo', 'minimal'] if args.profile == 'all' else [args.profile]):
            build = output / name
            build.mkdir(exist_ok=True)
            record = {'name': name, 'commands': [], 'status': 'failed'}
            report['profiles'].append(record)

            def execute(label, command):
                """Keep complete output per command; propagate any nonzero status."""
                print(name + ': ' + label, flush=True)
                entry = {'phase': label, 'argv': command, 'status': 'running'}
                record['commands'].append(entry)
                try:
                    with (build / (label + '.log')).open('w', encoding='utf-8') as log:
                        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
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
            execute('build', [args.cmake, '--build', str(build), '--config', 'Release',
                              '--parallel', str(args.jobs)])
            # Isolate CTest from inherited filters that could silently reduce evidence.
            if any(key in os.environ for key in ['CTEST_TESTS_TO_RUN', 'CTEST_TEST_LOAD']):
                raise RuntimeError('clear inherited CTest selection/load variables')
            execute('test', [args.ctest, '--test-dir', str(build), '-C', 'Release',
                             '--output-on-failure', '--no-tests=error', '--parallel', str(args.jobs)])
            record['status'] = 'passed'
        report['status'] = 'passed'
        return 0
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
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
