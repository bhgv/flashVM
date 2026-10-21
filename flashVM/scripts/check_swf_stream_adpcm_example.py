#!/usr/bin/env python3
"""Check actual C89 partial ADPCM export against hand-calculated differential sample values."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def main():
    """Test both native ownership modes, empty blocks, CLI and terminal output failures."""
    executable = str(Path(sys.argv[1]).resolve())
    # Width 4, predictor 100/index 0, codes 7/15/3/11; width 3, -100/index 12, codes 3/7.
    # Quantizer differences are +11,-30,+29,-25 and +39,-59, with one restart per block.
    expected = struct.pack('=8h', 100, 111, 81, 110, 85, -100, -61, -120)
    with tempfile.TemporaryDirectory(prefix='flashvm-counted-tutorial-') as directory:
        root = Path(directory)
        subprocess.run([executable], cwd=root, check=True, stdout=subprocess.PIPE)
        assert not list(root.iterdir())
        output = root / 'decoded.pcm'
        for args in ([str(output), '--wrong'], [str(output), '--ranges', 'extra']):
            invalid = subprocess.run([executable] + args, cwd=root,
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert invalid.returncode and not output.exists()
        for args in ([], ['--ranges']):
            output.write_bytes(b'old file must be replaced' * 20)
            result = subprocess.run([executable, str(output)] + args, cwd=root, check=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            assert 'Decoded 8 counted ADPCM frames after VM teardown.' in result.stdout
            assert output.read_bytes() == expected
            failed = subprocess.run([executable, str(root / 'missing' / 'out.pcm')] + args,
                                    cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert failed.returncode and not (root / 'missing').exists()
    print('PASS C89 counted ADPCM: prepared/range PCM, empty block, file replacement, CLI/errors')


if __name__ == '__main__':
    main()
