#!/usr/bin/env python3
"""Check the actual C89 streaming PCM tutorial and optional independent raw FFmpeg conversion.
Usage: python3 scripts/check_swf_stream_example.py BUILD/flashvm_examples_multimedia_swf_stream_main [--ffmpeg]
"""
import pathlib
import re
import struct
import subprocess
import sys
import tempfile


def main():
    tutorial = pathlib.Path(__file__).resolve().parents[1]/'examples/multimedia/swf_stream'
    source = (tutorial/'main.c').read_text()
    manual = (tutorial/'README.md').read_text()
    for path in [tutorial/'main.c',tutorial/'example_constants.h']:
        for line,text in enumerate(path.read_text().splitlines(),1):
            assert not text.strip() or '/*' in text or '*/' in text,(path,line)
    assert 'assert(' not in source
    for api in set(re.findall(r'\b(fvm_\w+)\s*\(',source)):
        assert re.search(r'\|[^\n]*'+api+r'[^\n]*\|',manual),api
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
        cursor = subprocess.run([executable, str(output), '--cursor'], cwd=directory,
                                check=True, stdout=subprocess.PIPE, universal_newlines=True)
        assert cursor.stdout == result.stdout and output.read_bytes() == expected
        converted = subprocess.run([executable, str(output), '--convert'], cwd=directory,
                                   check=True, stdout=subprocess.PIPE, universal_newlines=True)
        assert converted.stdout.splitlines() == result.stdout.splitlines()[:-1]+['Exported 4 sample frames from 3 blocks.']
        source = [(-32768,32512),(0,0),(32512,-32768)]
        values = []
        for n in range(4):
            left,phase = divmod(n*44100,48000)
            right = min(left+1,1) if left<2 else left  # Hold within the encoded block.
            for channel in range(2):
                numerator = source[left][channel]*(48000-phase)+source[right][channel]*phase
                values.append((-1 if numerator<0 else 1)*(abs(numerator)//48000))
        assert output.read_bytes() == struct.pack('=8h',*values)
        # Restore native-rate output before its independent raw FFmpeg comparison.
        subprocess.run([executable,str(output),'--cursor'],cwd=directory,check=True,stdout=subprocess.PIPE)
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
        result = subprocess.run([executable, str(missing), '--cursor'], cwd=directory,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not missing.exists() and not missing.parent.exists()
        result = subprocess.run([executable, str(missing), '--convert'], cwd=directory,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not missing.exists() and not missing.parent.exists()
    print('PASS streaming tutorial: exact PCM, frame gap, empty block, replacement and native error cleanup')
    return 0


if __name__ == '__main__':
    sys.exit(main())
