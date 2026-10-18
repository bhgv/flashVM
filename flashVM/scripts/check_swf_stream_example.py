#!/usr/bin/env python3
"""Check the actual C89 streaming PCM tutorial and optional independent raw FFmpeg conversion.
Usage: python3 scripts/check_swf_stream_example.py BUILD/flashvm_examples_multimedia_swf_stream_main [--ffmpeg]
"""
import pathlib
import struct
import subprocess
import sys
import tempfile


def main():
    executable = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        subprocess.run([executable], cwd=directory, check=True, stdout=subprocess.PIPE)
        assert not list(root.iterdir())
        output = root/'stream.pcm'
        output.write_bytes(b'previous content must be replaced')
        result = subprocess.run([executable, str(output)], cwd=directory, check=True,
                                stdout=subprocess.PIPE, universal_newlines=True)
        assert result.stdout.splitlines() == [
            'Frame 1: 2 sample frames', 'Frame 3: 1 sample frames',
            'Frame 4: 0 sample frames', 'Exported 3 sample frames from 3 blocks.']
        expected = struct.pack('=6h', -32768, 32512, 0, 0, 32512, -32768)
        assert output.read_bytes() == expected
        prepared = subprocess.run([executable, str(output), '--prepared'], cwd=directory,
                                  check=True, stdout=subprocess.PIPE, universal_newlines=True)
        assert prepared.stdout == result.stdout and output.read_bytes() == expected
        if '--ffmpeg' in sys.argv[2:]:
            # Decode independently known raw payload, not a reimplementation of the SWF parser.
            (root/'source.u8').write_bytes(bytes([0, 255, 128, 128, 255, 0]))
            fmt = 's16le' if sys.byteorder == 'little' else 's16be'
            subprocess.run(['ffmpeg', '-v', 'error', '-f', 'u8', '-ar', '44100', '-ac', '2',
                            '-i', str(root/'source.u8'), '-f', fmt, str(root/'oracle.pcm')], check=True)
            assert output.read_bytes() == (root/'oracle.pcm').read_bytes()
            print('PASS independent FFmpeg raw U8 to native S16 stereo oracle')
        missing = root/'missing'/'output.pcm'
        result = subprocess.run([executable, str(missing)], cwd=directory,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not missing.exists() and not missing.parent.exists()
        result = subprocess.run([executable, str(missing), '--prepared'], cwd=directory,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not missing.exists() and not missing.parent.exists()
    print('PASS streaming tutorial: exact PCM, frame gap, empty block, replacement and native error cleanup')
    return 0


if __name__ == '__main__':
    sys.exit(main())
