#!/usr/bin/env python3
"""Actual SDL-bound ADPCM packets: independent constant-code samples and rational conversion."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag
from stream_segments import converted, movie as pcm_movie


def block(width, channels, packets, initial):
    bits = format(width-2, '02b')
    samples = []
    delta = 7 // (1 << (width-1))
    for packet in range(packets):
        starts = [initial + 100*packet, -initial - 100*packet][:channels]
        bits += ''.join(format(value & 65535, '016b')+'000000' for value in starts)
        bits += '0' * ((4096-1)*channels*width)
        for frame in range(4096):
            values = tuple(min(32767, start+delta*frame) for start in starts)
            samples.append(values if channels == 2 else values*2)
    bits += '0' * (-len(bits) % 8)
    return bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8)), samples


def movie(width, channels, compressed):
    first, samples1 = block(width, channels, 1, 200)
    second, samples2 = block(width, channels, 2, 400)
    tags = tag(18, bytes([0x0e, 0x1e | (channels-1), 1, 0]))
    tags += tag(19, first)+tag(1)+tag(19)+tag(1)+tag(19, second)+tag(1)
    tail = [(-3000, 3000), (0, 0), (3000, -3000)]
    tags += tag(45, b'\x0e\x3b\x01\0')+tag(19, b''.join(struct.pack('<hh', *p) for p in tail))+tag(1)+tag(0)
    body = rect(0, 640, 0, 640)+struct.pack('<HH', 12*256, 4)+tags
    wire = (b'CWS' if compressed else b'FWS')+b'\x08'+struct.pack('<I', len(body)+8)
    wire += zlib.compress(body) if compressed else body
    expected = converted([samples1, [], samples2], 44100)+converted([tail], 22050)
    return wire, expected


def main():
    with tempfile.TemporaryDirectory(prefix='flashvm-adpcm-player-') as directory:
        root = Path(directory)
        for width in range(2, 6):
            for channels in (1, 2):
                for mode in ('--interpreter', '--jit'):
                    for compressed in (False, True):
                        wire, expected = movie(width, channels, compressed)
                        source = root/'source.swf'
                        source.write_bytes(wire)
                        base = root/'trace'
                        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software',
                                   SDL_AUDIODRIVER='dummy', FVM_AUDIO_TRACE=str(base))
                        result = subprocess.run([sys.argv[1], mode, '--once', '--stats', str(source)],
                                                env=env, capture_output=True, text=True, timeout=10)
                        if mode == '--jit' and 'requested JIT backend unavailable' in result.stderr:
                            continue
                        assert result.returncode == 0, result.stderr
                        assert result.stderr.count('FlashVM audio: driver=') == 1, result.stderr
                        with wave.open(str(base)+'.wav') as audio:
                            actual = audio.readframes(audio.getnframes())
                        assert actual == expected, (width, channels, mode, compressed, len(actual), len(expected))
                        report = Path(str(base)+'.txt').read_text()
                        events = [line.split()[1] for line in report.splitlines() if len(line.split()) == 5 and line.split()[0].isdigit()]
                        assert events.count('stream_segment') == 1 and events.count('clear') == 1, report
                        assert 'truncated=0 dropped_events=0' in report
    # Force host deadlines far ahead of the audio device: finite completion must preserve PCM.
    with tempfile.TemporaryDirectory(prefix='flashvm-fast-stream-tail-') as directory:
        root = Path(directory)
        adpcm_wire, adpcm_expected = movie(4,1,False)
        pcm_wire, pcm_expected = pcm_movie(False)
        _, first_samples = block(4,1,1,200)
        _, later_samples = block(4,1,2,400)
        # A gate stops future block writes, not continuous interpolation's immutable lookahead.
        # Compute the whole same-header grid independently, then select only authorized frames.
        isolated = converted([first_samples],44100)
        authorized = converted([first_samples, [], later_samples],44100)[:len(isolated)]
        assert authorized[:-4] == isolated[:-4] and authorized[-4:] != isolated[-4:]
        for name, wire, expected, frames in [('adpcm',adpcm_wire,adpcm_expected,4),
                                              ('pcm',pcm_wire,pcm_expected,8),
                                              ('future-gate',adpcm_wire,authorized,1)]:
            source = root/(name+'.swf')
            source.write_bytes(wire)
            base = root/name
            env = dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',SDL_AUDIODRIVER='dummy',FVM_AUDIO_TRACE=str(base))
            result = subprocess.run([sys.argv[1],'--interpreter','--frames',str(frames),'--fps','1000',str(source)],env=env,capture_output=True,text=True,timeout=10)
            assert result.returncode == 0,result.stderr
            with wave.open(str(base)+'.wav') as audio:
                actual = audio.readframes(audio.getnframes())
            assert actual == expected,(name,len(actual),len(expected))
        print('PASS fast finite PCM/ADPCM completion and exclusion of unauthorized future frames')
    if '--ffmpeg' in sys.argv[2:]:
        # Decode the bundled encoder-generated resource independently, one FLV audio tag per block.
        with tempfile.TemporaryDirectory(prefix='flashvm-adpcm-tutorial-oracle-') as directory:
            root = Path(directory)
            source = Path(__file__).resolve().parents[2]/'examples/player/pcm_stream/adpcm-stream.swf'
            wire = source.read_bytes()
            cursor = 8+(5+(wire[8] >> 3)*4+7)//8+4
            blocks = []
            while cursor < len(wire):
                head, = struct.unpack_from('<H', wire, cursor)
                cursor += 2
                size = head & 63
                if size == 63:
                    size, = struct.unpack_from('<I', wire, cursor)
                    cursor += 4
                payload = wire[cursor:cursor+size]
                cursor += size
                if head >> 6 != 19:
                    continue
                audio = b'\x1a'+payload
                flvtag = b'\x08'+len(audio).to_bytes(3, 'big')+bytes(7)+audio
                (root/'input.flv').write_bytes(b'FLV\x01\x04\0\0\0\x09'+bytes(4)+flvtag+struct.pack('>I',len(flvtag)))
                subprocess.run(['ffmpeg','-v','error','-i',str(root/'input.flv'),'-f','s16le','-y',str(root/'decoded.pcm')],check=True)
                samples = [p[0] for p in struct.iter_unpack('<h',(root/'decoded.pcm').read_bytes())]
                assert len(samples) >= 4096
                blocks.append([(value,value) for value in samples[:4096]])
            assert len(blocks) == 8
            expected = converted(blocks,22050)
            base = root/'trace'
            env = dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',SDL_AUDIODRIVER='dummy',FVM_AUDIO_TRACE=str(base))
            result = subprocess.run([sys.argv[1],'--interpreter','--once',str(source)],env=env,capture_output=True,text=True,timeout=10)
            assert result.returncode == 0,result.stderr
            with wave.open(str(base)+'.wav') as audio:
                actual = audio.readframes(audio.getnframes())
            assert actual == expected,(len(actual),len(expected))
            print('PASS bundled ADPCM tutorial: independent FFmpeg + rational conversion,',len(actual)//4,'accepted stereo frames')
    print('PASS actual ADPCM player: all code widths/channels, FWS/CWS, interpreter/JIT, PCM successor and exact accepted samples')


if __name__ == '__main__':
    main()
