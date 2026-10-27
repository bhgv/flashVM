"""Run an independent fixture against the portable filesystem C89 API."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile


# The binary is built against the selected port, not a VM. Own native test
# files; create a sparse extent and an invalid-byte name only as external oracles.
def main():
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='filesystem port ') as temporary:
        root = Path(temporary)
        data = root / 'каталог😀'
        data.mkdir()
        file = data / 'данные😀.bin'
        file.write_bytes(b'A\0B\xff')
        large = data / 'large'
        with large.open('wb') as output:
            output.truncate(4294967303)
        for index in range(70):
            (data / ('item%02d' % index)).write_bytes(b'x')
        bad = root / 'invalid-name'
        bad.mkdir()
        if os.name != 'nt':
            descriptor = os.open(os.fsencode(bad) + b'/invalid-\xff', os.O_CREAT | os.O_WRONLY, 0o600)
            os.close(descriptor)
        result = subprocess.run([str(binary), str(data), str(file), str(large), str(data / 'moved'), str(bad)],
                                text=True, capture_output=True, timeout=30)
        output = result.stdout + result.stderr
        if result.returncode or 'PASS filesystem C89 port' not in output or 'runtime error:' in output or 'ERROR: AddressSanitizer' in output:
            raise RuntimeError(output)
        print(output)


if __name__ == '__main__':
    main()
