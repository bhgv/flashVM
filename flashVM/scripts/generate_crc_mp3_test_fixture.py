#!/usr/bin/env python3
"""Generate original CRC-protected LAME MP3 with independent mpg123 S16 reference positions."""
import argparse
from pathlib import Path
import tempfile
from generate_free_mp3_test_fixture import Encoder, POSITIONS
from free_mp3_reference import Reference

CONFIGS = [(44100, 1, 128, False), (44100, 2, 128, False),
           (22050, 1, 64, False), (22050, 2, 64, False),
           (11025, 1, 32, False), (11025, 2, 32, False),
           (48000, 2, 400, True), (22050, 1, 80, True)]


def generate():
    """Keep encoder-produced CRC bytes unchanged; neither encoder nor oracle calls FlashVM."""
    encoder, decoder = Encoder(), Reference()
    lines = ['/* Original LAME 3.100 protected MP3; independent mpg123 golden positions. */',
             '#ifndef FVM_TEST_SWF_CRC_MP3_FIXTURE_HPP', '#define FVM_TEST_SWF_CRC_MP3_FIXTURE_HPP',
             '#include <cstddef>', 'namespace crc_mp3_fixture {',
             'static const size_t positions[] = {'+','.join(map(str, POSITIONS))+'};']
    with tempfile.TemporaryDirectory(prefix='flashvm-crc-vectors-') as directory:
        for index, (rate, channels, kbps, free) in enumerate(CONFIGS):
            data = encoder.encode(rate, channels, kbps, free_format=free, crc=True)
            samples = decoder.decode(Path(directory), (data, rate, channels, kbps, POSITIONS, []), latency=0)
            lines.append(f'static const unsigned char encoded{index}[] = {{')
            for offset in range(0, len(data), 20):
                lines.append(','.join(map(str, data[offset:offset+20]))+',')
            lines.append('};')
            golden = [samples[position][channel] for position in POSITIONS for channel in range(channels)]
            lines.append(f'static const short golden{index}[] = {{'+','.join(map(str, golden))+'};')
    lines += ['struct Fixture { const unsigned char *data; size_t size; unsigned rate, channels, kbps, packet_samples; const short *golden; };',
              'static const Fixture fixtures[] = {']
    for index, (rate, channels, kbps, _) in enumerate(CONFIGS):
        lines.append('{encoded%d,sizeof(encoded%d),%d,%d,%d,%d,golden%d},' %
                     (index, index, rate, channels, kbps, 1152 if rate >= 32000 else 576, index))
    return '\n'.join(lines+['};', '}', '#endif'])+'\n'


def main():
    """Require explicit output or compare the complete checked-in original fixture."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--output', type=Path)
    mode.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = generate()
    if args.check:
        target = Path(__file__).resolve().parents[1]/'tests/swf_crc_mp3_fixture.hpp'
        assert target.read_text() == result, 'Review codec/library versions before replacing CRC goldens'
        print('PASS original protected MP3 fixture reproduces exactly')
    else:
        args.output.write_text(result)


if __name__ == '__main__':
    main()
