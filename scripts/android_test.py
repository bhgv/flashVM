#!/usr/bin/env python3
"""CTest execution adapter for one Android executable on an explicitly chosen device.

Set CMAKE_CROSSCOMPILING_EMULATOR to Python;this-script;--serial;SERIAL;
--directory;/data/local/tmp/flashvm-SESSION. Tests in one build share that directory
so CTest file fixtures survive across processes. Use a different directory for
each simultaneous build. This adapter never selects a device implicitly.
"""
import argparse
from pathlib import Path
import shlex
import subprocess
import sys


def run_test(adb, serial, directory, executable, arguments, timeout):
    """Stage and execute a target; preserve ADB errors and the remote exit status."""
    executable = Path(executable).resolve(strict=True)
    if not executable.is_file():
        raise ValueError('target must be a regular executable file')
    # Limit cleanup/staging to a dedicated child directory on the test device.
    parts = directory.split('/')
    if (not directory.startswith('/data/local/tmp/flashvm-') or
            any(part in ('.', '..', '') for part in parts[1:]) or
            len(parts) != 5 or directory == '/data/local/tmp/flashvm-'):
        raise ValueError('directory must be /data/local/tmp/flashvm-SESSION')
    command = [adb, '-s', serial]
    remote = directory + '/' + executable.name
    subprocess.run(command + ['get-state'], check=True, timeout=timeout)
    subprocess.run(command + ['shell', '-T', 'mkdir -p ' + shlex.quote(directory)],
                   check=True, timeout=timeout)
    subprocess.run(command + ['push', str(executable), remote], check=True, timeout=timeout)
    # ADB shell protocol v2 returns the remote exit code. Do not use shell -x,
    # which disables that protocol, or a trailing command that masks failures.
    shell = ('chmod 700 ' + shlex.quote(remote) + ' && cd ' + shlex.quote(directory) +
             ' && exec ' + ' '.join(shlex.quote(value) for value in [remote] + arguments))
    return subprocess.run(command + ['shell', '-T', shell], timeout=timeout).returncode


def main():
    """Parse CMake's appended target/arguments and map transport errors to failure."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adb', default='adb')
    parser.add_argument('--serial', required=True)
    parser.add_argument('--directory', required=True)
    parser.add_argument('--timeout', type=float, default=25)
    parser.add_argument('executable', type=Path)
    parser.add_argument('arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.timeout <= 0 or not args.serial.strip():
        parser.error('positive timeout and nonempty device serial required')
    try:
        return run_test(args.adb, args.serial, args.directory, args.executable,
                        args.arguments, args.timeout)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print('Android target execution failed: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
