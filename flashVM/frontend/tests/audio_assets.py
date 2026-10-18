"""Independently decode shipped SWF ADPCM and check full-amplitude block restarts.

This oracle never imports the fixture encoder or calls the VM decoder. The
analytic source waveform exposes quantizer overload even when two implementations
would otherwise agree on the same audible defect.
"""
import math
from pathlib import Path
import struct


# Standard four-bit IMA/SWF decoder tables, independent of the fixture generator.
STEPS = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
    4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
    24623, 27086, 29794, 32767,
)
INDEX = (-1, -1, -1, -1, 2, 4, 6, 8)


class Bits:
    """Read unsigned MSB-first SWF bit fields with an explicit truncation check."""

    def __init__(self, data):
        self.data = data
        self.position = 0

    def read(self, count):
        """Consume exactly count bits, including non-byte-aligned block headers."""
        assert self.position + count <= len(self.data) * 8
        result = 0
        for _ in range(count):
            byte, bit = divmod(self.position, 8)
            result = (result << 1) | ((self.data[byte] >> (7 - bit)) & 1)
            self.position += 1
        return result


def sound_payload(movie):
    """Locate resource nine in an uncompressed SWF8 without using its authoring code."""
    assert movie[:4] == b'FWS\x08'
    assert struct.unpack_from('<I', movie, 4)[0] == len(movie)
    position = 8 + (5 + 4 * (movie[8] >> 3) + 7) // 8 + 4
    while position < len(movie):
        header = struct.unpack_from('<H', movie, position)[0]
        position += 2
        size = header & 63
        if size == 63:
            size = struct.unpack_from('<I', movie, position)[0]
            position += 4
        payload = movie[position:position + size]
        assert len(payload) == size
        position += size
        if header >> 6 == 14 and struct.unpack_from('<H', payload)[0] == 9:
            assert payload[2] == 0x1e  # Mono, 44.1 kHz, 16-bit ADPCM.
            assert struct.unpack_from('<I', payload, 3)[0] == 11025
            return payload[7:]
    raise AssertionError('Missing ADPCM tone')


def decode(payload):
    """Decode real 4096-frame SWF blocks, including each absolute first sample."""
    bits = Bits(payload)
    assert bits.read(2) + 2 == 4
    output = []
    for frame in range(11025):
        if frame % 4096 == 0:
            predictor = bits.read(16)
            if predictor & 32768:
                predictor -= 65536
            index = bits.read(6)
        else:
            code = bits.read(4)
            step = STEPS[index]
            difference = step >> 3
            for mask, amount in ((4, step), (2, step >> 1), (1, step >> 2)):
                if code & mask:
                    difference += amount
            predictor += -difference if code & 8 else difference
            predictor = max(-32768, min(32767, predictor))
            index = max(0, min(88, index + INDEX[code & 7]))
        output.append(predictor)
    return output


def main():
    """Catch full-level restart overload in all three shipped playback fixtures."""
    folder = Path(__file__).resolve().parents[2] / 'examples/player/animation_audio'
    paths = sorted(folder.glob('*.swf'))
    assert len(paths) == 3, paths
    for path in paths:
        samples = decode(sound_payload(path.read_bytes()))
        for boundary in (4096, 8192):
            window = range(boundary - 16, boundary + 64)
            error = max(abs(samples[i] - round(5000 * math.sin(2 * math.pi * 880 * i / 44100)))
                        for i in window)
            jump = max(abs(samples[i] - samples[i - 1]) for i in window)
            # The intended sinusoid changes by at most 627 per source frame.
            # Quantization has a margin; the old index-zero restart exceeds both bounds.
            assert error < 250, (path.name, boundary, error)
            assert jump < 900, (path.name, boundary, jump)
            print(path.name, 'boundary', boundary, 'error', error, 'step', jump)


if __name__ == '__main__':
    main()
