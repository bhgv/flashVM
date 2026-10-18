#!/usr/bin/env python3
"""Create three independent SWF8 wire fixtures; no ActionScript compiler is required."""
import math
from pathlib import Path
import struct

WIDTH, HEIGHT, FPS, FRAMES = 640, 360, 24, 120
TWIPS = 20
DEFINE_SHAPE3, PLACE_OBJECT2, DO_ACTION, SHOW_FRAME = 32, 26, 12, 1
DEFINE_SOUND, START_SOUND, BACKGROUND, END = 14, 15, 9, 0


class Bits:
    """Write SWF's MSB-first packed signed fields without using production code."""
    def __init__(self):
        self.values = []

    def put(self, value, count):
        self.values.extend((value >> shift) & 1 for shift in range(count - 1, -1, -1))

    def bytes(self):
        data = self.values + [0] * (-len(self.values) % 8)
        return bytes(sum(data[i + j] << (7 - j) for j in range(8))
                     for i in range(0, len(data), 8))


def rect(width, height):
    """Represent a zero-origin stage/shape using signed 16-bit twips."""
    bits = Bits()
    bits.put(16, 5)
    for value in (0, width * TWIPS, 0, height * TWIPS):
        bits.put(value, 16)
    return bits.bytes()


def matrix(x, y):
    """Identity scale/rotation plus a signed pixel translation."""
    bits = Bits()
    bits.put(0, 1)
    bits.put(0, 1)
    bits.put(16, 5)
    bits.put(round(x * TWIPS), 16)
    bits.put(round(y * TWIPS), 16)
    return bits.bytes()


def tag(code, payload=b''):
    """Use a long tag header only when the short six-bit length cannot hold the data."""
    return (struct.pack('<H', code << 6 | len(payload)) if len(payload) < 63 else
            struct.pack('<HI', code << 6 | 63, len(payload))) + payload


def shape(identity, color, width=32, height=32):
    """Encode one opaque solid rectangle, four straight edges, no line style."""
    bits = Bits()
    bits.put(1, 4)  # One bit addresses the single fill style.
    bits.put(0, 4)  # There are no line styles.
    bits.put(0, 1)  # Begin a style-change record.
    bits.put(3, 5)  # MoveTo plus FillStyle0.
    bits.put(1, 5)
    bits.put(0, 1)
    bits.put(0, 1)
    bits.put(1, 1)  # Select solid fill number one.
    for dx, dy in ((width * TWIPS, 0), (0, height * TWIPS),
                   (-width * TWIPS, 0), (0, -height * TWIPS)):
        bits.put(1, 1)  # Edge record.
        bits.put(1, 1)  # Straight rather than quadratic edge.
        bits.put(10, 4)  # Twelve signed bits per delta.
        bits.put(1, 1)  # General edge, both deltas present.
        bits.put(dx, 12)
        bits.put(dy, 12)
    bits.put(0, 6)  # EndShapeRecord.
    payload = struct.pack('<H', identity) + rect(width, height)
    payload += bytes([1, 0, *color, 0]) + bits.bytes()  # One RGBA fill, zero lines.
    return tag(DEFINE_SHAPE3, payload)


def place(depth, identity, name, x, y):
    """Publish a named clip so ordinary AVM1 Get/SetProperty can move it."""
    return tag(PLACE_OBJECT2, bytes([0x26]) + struct.pack('<HH', depth, identity) +
               matrix(x, y) + name.encode('ascii') + b'\0')


def move(depth, x, y):
    """Update the existing instance without replacing its script properties."""
    return tag(PLACE_OBJECT2, bytes([5]) + struct.pack('<H', depth) + matrix(x, y))


def push(value):
    """Encode a single AVM1 Push string or signed integer."""
    data = (b'\0' + value.encode('ascii') + b'\0' if isinstance(value, str) else
            b'\x07' + struct.pack('<i', value))
    return b'\x96' + struct.pack('<H', len(data)) + data


def host(name):
    """Read one scalar from the player-provided HostInput object."""
    return push('HostInput') + b'\x1c' + push(name) + b'\x4e'


def key_down(code):
    """Call the standard Key polling subset: argument, count, receiver, method, CallMethod."""
    return push(code) + push(1) + push('Key') + b'\x1c' + push('isDown') + b'\x52'


def key_code():
    """Read the latest pressed virtual key through the real Key.getCode native bridge."""
    return push(0) + push('Key') + b'\x1c' + push('getCode') + b'\x52'


def set_position(target, index, expression):
    """Leave target/index/value on the AVM1 stack, then execute SetProperty."""
    return push(target) + push(index) + expression + b'\x23'


def actions():
    """Move the marker to the pointer; move the blue box with held arrows/space."""
    data = set_position('pointer', 0, host('x')) + set_position('pointer', 1, host('y'))
    for index, positive, negative in ((0, 39, 37), (1, 40, 38)):
        old = push('box') + push(index) + b'\x22'
        delta = key_down(positive) + key_down(negative) + b'\x0b' + push(5) + b'\x0c'
        data += set_position('box', index, old + delta + b'\x0a')
    # Alpha and scale make short taps visible through cumulative host counters.
    data += set_position('pointer', 2, push(100) + host('buttons') + push(10) + b'\x0c\x0a')
    data += set_position('box', 2, push(100) + key_down(32) + push(80) + b'\x0c\x0a')
    data += set_position('key', 0, push(20) + key_code() + push(4) + b'\x0c\x0a')
    data += set_position('click', 0, push(20) + host('clicks') + push(20) + b'\x0c\x0a')
    return tag(DO_ACTION, data + b'\0')


def sound(identity, rate, frequency, stereo=False, eight_bit=False):
    """Produce a quiet 250 ms sine diagnostic with fade-in/out, no downloaded media."""
    count = rate // 4
    pcm = bytearray()
    for frame in range(count):
        gain = min(1.0, frame / 128, (count - 1 - frame) / 128)
        left = math.sin(2 * math.pi * frequency * frame / rate) * gain
        right = math.sin(2 * math.pi * frequency * 1.5 * frame / rate) * gain
        for sample in ((left, right) if stereo else (left,)):
            pcm += bytes([round(128 + sample * 25)]) if eight_bit else struct.pack('<h', round(sample * 6000))
    rate_code = {11025: 1, 22050: 2, 44100: 3}[rate]
    # Format 0/U8 or format 3/explicit little-endian S16 keeps endian behavior unambiguous.
    flags = (0 if eight_bit else 3 << 4) | rate_code << 2 | (0 if eight_bit else 2) | int(stereo)
    return tag(DEFINE_SOUND, struct.pack('<HBI', identity, flags, count) + pcm)


# Standard IMA/SWF four-bit adaptive quantizer: immutable step and index tables.
ADPCM_STEPS = (7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,
               60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,
               337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,
               1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,
               4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,
               12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767)
ADPCM_INDEX = (-1,-1,-1,-1,2,4,6,8)


def adpcm_sound():
    """Encode a quiet 880 Hz mono tone into SWF ADPCM with real 4096-frame block headers."""
    frames = 11025
    bits = Bits()
    bits.put(2, 2)  # ADPCMCodeSize + 2 means four-bit differential codes.
    predictor, index = 0, 0
    for frame in range(frames):
        gain = min(1.0, frame / 128, (frames - frame - 1) / 128)
        sample = round(5000 * gain * math.sin(2 * math.pi * 880 * frame / 44100))
        if frame % 4096 == 0:
            # Restart the absolute predictor, but retain the adapted step size: resetting
            # it to seven on a full-amplitude tone overloads the next differential codes.
            predictor = sample
            bits.put(predictor, 16)
            bits.put(index, 6)
            continue
        step = ADPCM_STEPS[index]
        delta = sample - predictor
        code = 8 if delta < 0 else 0
        magnitude = abs(delta)
        reconstructed = step >> 3
        for mask, threshold in ((4, step), (2, step >> 1), (1, step >> 2)):
            if magnitude >= threshold:
                code |= mask
                magnitude -= threshold
                reconstructed += threshold
        predictor += -reconstructed if code & 8 else reconstructed
        predictor = max(-32768, min(32767, predictor))
        index = max(0, min(88, index + ADPCM_INDEX[code & 7]))
        bits.put(code, 4)
    return tag(DEFINE_SOUND, struct.pack('<HBI', 9, 0x1e, frames) + bits.bytes())


def movie(interactive, stop_sounds=False):
    """120 frames: imported animation and sound controls, optionally native AVM1 input logic."""
    data = tag(BACKGROUND, bytes([24, 28, 40]))
    colors = ((255, 160, 40, 255), (40, 140, 255, 255), (255, 80, 160, 255),
              (80, 255, 140, 255), (255, 240, 60, 255))
    for identity, color in enumerate(colors, 1):
        data += shape(identity, color)
    for identity in range(1, 6):
        # A named DefineSprite is a MovieClip; raw shape instances do not accept AVM1 clip properties.
        contents = tag(PLACE_OBJECT2, bytes([6]) + struct.pack('<HH', 1, identity) + matrix(0, 0))
        contents += tag(SHOW_FRAME) + tag(END)
        data += tag(39, struct.pack('<HH', 100 + identity, 1) + contents)
    data += sound(7, 11025, 440, eight_bit=True) + sound(8, 44100, 660, stereo=True)
    data += adpcm_sound()
    names = ('animation', 'box', 'pointer', 'key', 'click')
    for depth, name in enumerate(names, 1):
        data += place(depth, 100 + depth, name, 20, 60 * depth - 40)
    for frame in range(FRAMES):
        x = 20 + (frame % 60) * 9
        y = 20 + round(22 * math.sin(2 * math.pi * frame / 60))
        data += move(1, x, y)
        if not interactive:
            data += move(2, 580 - (frame % 60) * 9, 110)
        if frame in (0, 24, 72):
            identity = 7 if frame < 72 else 8
            flags = 4 if frame == 24 else 0  # HasLoops is bit 2.
            data += tag(START_SOUND, struct.pack('<HB', identity, flags) +
                        (struct.pack('<H', 4) if flags else b''))
        if frame == 26:
            data += tag(START_SOUND, struct.pack('<HB', 7, 16))  # SyncNoMultiple.
        if frame == 36:
            data += tag(START_SOUND, struct.pack('<HB', 7, 32))  # SyncStop truncates the loop.
        if frame == 96:
            data += tag(START_SOUND, struct.pack('<HB', 9, 0))
        data += actions() if interactive else tag(DO_ACTION, push(1) + b'\x17\0')
        if stop_sounds and frame in (0, 30):
            # Frame zero tests cancellation before any PCM was ever queued; frame30 stops a live loop.
            data += tag(DO_ACTION, b'\x09\0')
        data += tag(SHOW_FRAME)
    body = rect(WIDTH, HEIGHT) + struct.pack('<HH', FPS * 256, FRAMES) + data + tag(END)
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body


def main():
    """Regenerate only the three explicitly named sibling SWF files."""
    folder = Path(__file__).resolve().parent
    for name, interactive, stops in (('01-animation-audio.swf', False, False),
                                     ('02-input-audio.swf', True, False),
                                     ('03-stop-sounds.swf', False, True)):
        destination = folder / name
        destination.write_bytes(movie(interactive, stops))
        print(destination.name, destination.stat().st_size)


if __name__ == '__main__':
    main()
