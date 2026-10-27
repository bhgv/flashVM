"""Compare real Sys metadata/directory results with an independent host fixture."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


# Execute a borrowed tool command in an isolated work directory. Return output
# only on success; fail on VM FAIL output or sanitizer diagnostics as well.
def run(arguments, work):
    result = subprocess.run([str(value) for value in arguments], cwd=work, text=True, capture_output=True, timeout=90)
    output = result.stdout + result.stderr
    if result.returncode or 'FAIL' in output or 'runtime error:' in output or 'ERROR: AddressSanitizer' in output:
        raise RuntimeError(str(arguments) + '\n' + output)
    return output


# Build actual binary/Unicode/large-file fixtures and compile the shipped Limbo
# client. The client uses real Sys/Readdir, not a mocked directory implementation.
# Native temp files belong to this test only and disappear after every run.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('compiler', type=Path)
    parser.add_argument('runtime', type=Path)
    parser.add_argument('includes', type=Path)
    parser.add_argument('source', type=Path)
    parser.add_argument('readdir', type=Path)
    parser.add_argument('mode', choices=('jit', 'interpreter'))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='Sys filesystem ') as temporary:
        work = Path(temporary)
        root = work / 'каталог😀'
        root.mkdir()
        (root / 'данные😀.bin').write_bytes(b'A\0B\xff')
        (root / 'подкаталог').mkdir()
        for index in range(70):
            (root / ('item%02d' % index)).write_bytes(bytes([index]))
        large = root / 'large.bin'
        with large.open('wb') as output:
            output.truncate(4294967303)
        source = args.source.resolve().read_text().replace('FIXTURE_ROOT', json.dumps(str(root), ensure_ascii=False))
        (work / 'check.b').write_text(source)
        shutil.copyfile(args.readdir.resolve(), work / 'readdir.dis')
        run([args.compiler.resolve(), '-g', '-I', args.includes.resolve(), '-o', work / 'check.dis', work / 'check.b'], work)
        for quantum in (1, 2048):
            output = run([args.runtime.resolve(), '--jit' if args.mode == 'jit' else '--no-jit', '--quantum', quantum,
                          '--stats', '--max-instructions', 3000000, work / 'check.dis'], work)
            if 'PASS Sys filesystem: metadata, batches, Readdir, quota and errors' not in output:
                raise RuntimeError('missing actual filesystem checks:\n' + output)
        if (root / 'данные😀.bin').read_bytes() != b'A\0B\xff' or large.stat().st_size != 4294967303:
            raise RuntimeError('read-only metadata changed fixture contents')
    print('PASS hosted Sys filesystem', args.mode)


if __name__ == '__main__':
    main()
