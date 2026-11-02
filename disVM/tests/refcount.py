"""Exercise real native refcount primitives with atomic and registry-lock backends."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


# Function-scoped semantic controls change only a temporary primitive copy.
# They violate refusal/last-result contracts without executing undefined overflow.
def controlled(source, name):
    if name == 'last':
        begin = source.index('DVM_RefcountResult dvm_refcount_release_u(')
        end = source.index('int dvm_refcount_load_i(', begin)
        part = source[begin:end]
        marker = 'return value == 1 ? DVM_REFCOUNT_LAST : DVM_REFCOUNT_LIVE;'
        if part.count(marker) != 1:
            raise RuntimeError('last control drift')
        return source[:begin] + part.replace(marker, 'return DVM_REFCOUNT_LIVE;') + source[end:]
    signature, predicate = {
        'zero': ('retain_u', 'counter && dvm_refcount_load_u(counter) == 0'),
        'overflow': ('retain_u', 'counter && dvm_refcount_load_u(counter) == ULONG_MAX'),
        'underflow': ('release_u', 'counter && dvm_refcount_load_u(counter) == 0'),
        'negative': ('retain_i', 'counter && dvm_refcount_load_i(counter) < 0'),
    }[name]
    begin = source.index('DVM_RefcountResult dvm_refcount_' + signature + '(')
    at = source.index('{', begin) + 1
    return source[:at] + '\n    if (' + predicate + ') { return DVM_REFCOUNT_LIVE; }\n' + source[at:]


# Compile production primitives and actual OS ports. Boundary controls must reach
# semantic status10. Signal/sanitizer/compiler failures cannot count as detection.
def main():
    root = Path(__file__).resolve().parents[2]
    installed = not (root / 'disVM/host/refcount.c').exists()
    host = root / ('source/host' if installed else 'disVM/host')
    tests = root / ('source/tests' if installed else 'disVM/tests')
    include = root / ('include' if installed else 'disVM/include')
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    source = (host / 'refcount.c').read_text()
    with tempfile.TemporaryDirectory(prefix='native refcounts ') as directory:
        work = Path(directory)
        for backend in ('native', 'locked'):
            for control in (None, 'zero', 'overflow', 'underflow', 'negative', 'last'):
                path = work / 'count.c'
                path.write_text(controlled(source, control) if control else source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread']
                if backend == 'locked':
                    command += ['-DDVM_REFCOUNT_FORCE_LOCKED=1']
                command += ['-I' + str(include), '-I' + str(ports)]
                binary = work / 'count'
                subprocess.run(command + [str(tests / 'refcount.c'), str(path),
                    str(ports / 'linux/thread.c'),
                    str(ports / 'linux/channel_sync.c'), '-o', str(binary)],
                    check=True, capture_output=True, text=True)
                result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=90)
                if control:
                    if result.returncode != 10 or 'FAIL refcount predicate:' not in result.stderr:
                        raise RuntimeError('refcount control missed: ' + backend + '/' + control + result.stdout + result.stderr)
                elif result.returncode or (backend == 'locked' and 'refcounts: locked,' not in result.stdout):
                    raise RuntimeError(result.stdout + result.stderr)
                else:
                    print(result.stdout, end='')
        print('PASS ten boundary controls across both backends; no managed heap/GC concurrency claim')


if __name__ == '__main__':
    main()
