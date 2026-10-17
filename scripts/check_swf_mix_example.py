#!/usr/bin/env python3
"""Validate real bounded snapshot mixing and native stdio output.
Usage: python3 scripts/check_swf_mix_example.py BUILD/flashvm_examples_multimedia_swf_mix_main [--ffmpeg] [--native|--interpreter]
"""
import pathlib
import struct
import subprocess
import sys
import tempfile


def main():
    executable = str(pathlib.Path(sys.argv[1]).resolve())
    expected_mode = 'native' if '--native' in sys.argv[2:] else 'interpreter' if '--interpreter' in sys.argv[2:] else None
    mode_args = ['--jit'] if expected_mode == 'native' else ['--interpreter'] if expected_mode else []
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        subprocess.run([executable], cwd=directory, check=True, stdout=subprocess.PIPE)
        assert not list(root.iterdir())
        output = root/'mixed.pcm'
        output.write_bytes(b'previous contents must be replaced')
        result = subprocess.run([executable, str(output)] + mode_args, check=True, stdout=subprocess.PIPE, universal_newlines=True)
        assert 'Mixed 6 frames' in result.stdout
        if expected_mode:
            assert 'StopSounds execution: ' + expected_mode in result.stdout
        assert output.read_bytes() == struct.pack('=6h', -256, 0, -256, 32512, 0, -32768)
        if '--ffmpeg' in sys.argv[2:]:
            # Independent installed FFmpeg filter oracle; never invoked by FlashVM itself.
            (root/'a.pcm').write_bytes(struct.pack('=3h', -32768, 0, 32512))
            (root/'b.pcm').write_bytes(struct.pack('=6h', 32512, 0, -32768, 32512, 0, -32768))
            fmt = 's16le' if sys.byteorder == 'little' else 's16be'
            command = ['ffmpeg', '-v', 'error']
            for name in ('a.pcm', 'b.pcm'):
                command += ['-f', fmt, '-ar', '44100', '-ac', '1', '-i', str(root/name)]
            command += ['-filter_complex', 'amix=inputs=2:normalize=0:duration=longest:dropout_transition=0', '-f', fmt, '-y', str(root/'oracle.pcm')]
            subprocess.run(command, check=True)
            assert output.read_bytes() == (root/'oracle.pcm').read_bytes()
            print('PASS independent FFmpeg amix oracle: byte-exact unnormalized longest-voice result')
        rejected = root/'rejected.pcm'
        result = subprocess.run([executable, str(rejected), '--unknown-mode'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not rejected.exists()
        missing = root/'missing'/'output.pcm'
        result = subprocess.run([executable, str(missing)] + mode_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not missing.exists() and not missing.parent.exists()
    print('PASS actual C89 mixed output: exact six-frame sum, EOF padding, replacement and native error cleanup')
    return 0


if __name__ == '__main__':
    sys.exit(main())
