"""Run real directory owners and the IDE frontend against private native fixtures."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


# Borrow command/work/input, run with a deadline and return decoded output.
# Nonzero exits, VM failures and sanitizer diagnostics always fail the oracle.
def run(command, work, input_text=None):
    result = subprocess.run([str(value) for value in command], cwd=work, input=input_text,
                            text=True, capture_output=True, timeout=60)
    output = result.stdout + result.stderr
    if result.returncode or 'FAIL' in output or 'runtime error:' in output or 'ERROR: AddressSanitizer' in output:
        raise RuntimeError(str(command) + '\n' + output)
    return output


# Create actual Unicode/sparse/control-name files, compile the shipped tutorial
# and exercise service/console lifecycle. This driver owns only its temporary files.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runtime', 'compiler', 'includes', 'bridge', 'source', 'modules'):
        parser.add_argument(name, type=Path)
    parser.add_argument('mode', choices=('jit', 'interpreter'))
    args = parser.parse_args()
    option = '--jit' if args.mode == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='directory browser ') as temporary:
        work = Path(temporary)
        root = work / 'каталог😀'
        root.mkdir()
        invalid_names = work / 'invalid-names'
        invalid_names.mkdir()
        for index in range(3):
            (invalid_names / ('valid%02d' % index)).write_bytes(b'x')
        descriptor = os.open(os.fsencode(invalid_names) + b'/invalid-\xff', os.O_CREAT | os.O_WRONLY, 0o600)
        os.close(descriptor)
        (root / 'данные😀.bin').write_bytes(b'A\0B\xff')
        (root / 'подкаталог').mkdir()
        (root / 'control\x1b[31m\nname').write_bytes(b'control')
        for index in range(70):
            (root / ('item%02d' % index)).write_bytes(b'x')
        with (root / 'large.bin').open('wb') as output:
            output.truncate(4294967303)
        for module in args.modules.resolve().glob('*.dis'):
            shutil.copyfile(module, work / module.name)
        source = args.source.resolve().read_text().replace('FIXTURE_ROOT', json.dumps(str(root), ensure_ascii=False))
        (work / 'browser.b').write_text(source)
        run([args.compiler.resolve(), '-g', '-I', args.includes.resolve(), '-I', args.bridge.resolve(),
             '-o', work / 'browser.dis', work / 'browser.b'], work)
        for quantum in (1, 2048):
            output = run([args.runtime.resolve(), option, '--quantum', quantum, '--stats',
                          '--max-instructions', 5000000, work / 'browser.dis'], work)
            if 'PASS directory browser: channels, batches, identities, quota and shutdown' not in output:
                raise RuntimeError('missing service acceptance:\n' + output)
            counters = re.search(r'instructions=(\d+) native=(\d+)', output)
            if not counters or (int(counters[2]) > 0) != (args.mode == 'jit'):
                raise RuntimeError('wrong service dispatch counters:\n' + output)
        commands = ('set untouched\nstatus\ndir ' + str(root) + '\ndir ' + str(root / 'missing') +
                    '\ndirnext\ndirnext\ndirnext\ndirnext\ndirclose\nstat ' + str(root / 'данные😀.bin') +
                    '\nstat ' + str(root / 'large.bin') + '\nshow\nquit!\n')
        output = run([args.runtime.resolve(), option, '--stats', work / 'editor-console.dis'], work, commands)
        if re.findall(r'DIRECTORY BATCH (\d+)', output) != ['32', '32', '10']:
            raise RuntimeError('console batch/preserved cursor mismatch:\n' + output)
        for expected in ('ERROR dir:', 'DIRECTORY END', 'DIRECTORY CLOSED', 'size=4294967303',
                         'TEXT 9\nuntouched\nEND', 'BYE document owner stopped', '\\u001b', '\\u000a'):
            if expected not in output:
                raise RuntimeError('missing console oracle ' + repr(expected) + ':\n' + output)
        if '\x1b' in output:
            raise RuntimeError('native filename reached the terminal as a control sequence')
        # Shutdown must retire a still-open cursor as well as explicitly closed ones.
        output = run([args.runtime.resolve(), option, '--quantum', 1, work / 'editor-console.dis'],
                     work, 'dir ' + str(root) + '\nquit\n')
        if 'BYE document owner stopped' not in output:
            raise RuntimeError('active directory owner did not shut down:\n' + output)
    print('PASS directory service and IDE browser', args.mode)


if __name__ == '__main__':
    main()
