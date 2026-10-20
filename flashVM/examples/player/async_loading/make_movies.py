#!/usr/bin/env python3
"""Generate the commented async SWF8 tutorial using only Python standard-library bytes."""
from pathlib import Path
import struct

UNDEFINED = b"\x96\x01\x00\x03"

class Bits:
    """Independent MSB-first fixture writer; production RECT/shape readers are not reused."""
    def __init__(self):
        """Start an empty MSB-first bit stream."""
        self.values = []

    def put(self, value, count):
        """Append exactly count low bits, most significant bit first."""
        self.values.extend((value >> shift) & 1 for shift in range(count - 1, -1, -1))

    def bytes(self):
        """Pad the final byte with zero bits and pack the stream."""
        values = self.values + [0] * (-len(self.values) % 8)
        return bytes(sum(values[i + j] << (7 - j) for j in range(8))
                     for i in range(0, len(values), 8))

def rect(xmin, xmax, ymin, ymax):
    """Use signed sixteen-bit twip fields, including nonzero/negative authored stage origins."""
    bits = Bits()
    bits.put(16, 5)
    for value in (xmin, xmax, ymin, ymax):
        bits.put(value, 16)
    return bits.bytes()

def tag(code, payload=b''):
    """Encode short/long records without depending on VM serialization."""
    if len(payload) < 63:
        return struct.pack('<H', code << 6 | len(payload)) + payload
    return struct.pack('<HI', code << 6 | 63, len(payload)) + payload

def push(*values):
    """Encode ActionPush string/int values with counted little-endian payload."""
    data = b''
    for value in values:
        data += b'\0' + value.encode() + b'\0' if isinstance(value, str) else b'\x07' + struct.pack('<i', value)
    return b'\x96' + struct.pack('<H', len(data)) + data

def global_call(name, *args):
    """Push reversed arguments, call a global by name and discard its result."""
    return push(*reversed(args), len(args), name) + b'\x3d\x17'

def method_call(owner, name, *args):
    """Resolve the receiver, invoke its method and discard its result."""
    return push(*reversed(args), len(args), owner) + b'\x1c' + push(name) + b'\x52\x17'

def swf(frames):
    """Wrap action frames in an uncompressed SWF8 movie with explicit End tags."""
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, len(frames))
    for actions in frames:
        body += tag(12, actions + b'\0') + tag(1)
    body += tag(0)
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body

def require(path, expected):
    """Throw a diagnostic unless a variable strictly equals the expected value."""
    failure = push('loaded value mismatch: ' + path) + b'\x2a'
    return push(path) + b'\x1c' + push(expected) + b'\x66\x9d\x02\0' + struct.pack('<h', len(failure)) + failure

def child_movie():
    """Load form data relative to the child source and stop its timeline."""
    return swf([method_call('this', 'loadVariables', 'child-vars.txt') + b'\x07'])

def get(name):
    """Read a variable through ActionGetVariable."""
    return push(name) + b'\x1c'

def equal(name, value):
    """Compare a variable and literal using ActionStrictEquals."""
    return get(name) + push(value) + b'\x66'

def when(condition, body):
    """If true skip one unconditional jump; otherwise jump over exactly this body."""
    return condition + b'\x9d\x02\x00\x05\x00\x99\x02\x00' + struct.pack('<h', len(body)) + body

def root_movie(prefix=''):
    """Two looping wait frames avoid reissuing the first frame's resource requests."""
    first = push('_root.iter', 0) + b'\x1d'
    first += method_call('_root', 'createEmptyMovieClip', 'child', 1)
    first += global_call('loadMovie', prefix + 'nested/child.swf', 'child')
    first += method_call('_root', 'loadVariables', prefix + 'root-vars.txt')
    pulse = push('_root.timer_ran', 1) + b'\x1d'
    pulse += UNDEFINED + b'\x3e\x00'
    definition = b'pulse\0\0\0' + struct.pack('<H', len(pulse)) + pulse
    first += b'\x9b' + struct.pack('<H', len(definition)) + definition
    first += push('_root.timer') + push(10) + get('pulse') + push(2, 'setInterval') + b'\x3d\x1d'
    second = push('_root.frame_ran', 1) + b'\x1d'
    completed = require('_root.greeting', 'hello from root') + require('_root.child.answer', 'child origin works')
    completed += require('_root.timer_ran', 1) + require('_root.frame_ran', 1)
    completed += push('_root.finished', 1) + b'\x1d'
    completed += get('_root.timer') + push(1, 'clearInterval') + b'\x3d\x17'
    completed += b'\x07'
    third = push('_root.iter') + get('_root.iter') + push(1) + b'\x47\x1d'
    third += when(get('_root.iter') + push(200) + b'\x67', require('_root.finished', 1))
    ready = equal('_root.greeting', 'hello from root')
    ready += equal('_root.child.answer', 'child origin works') + b'\x10'
    ready += equal('_root.timer_ran', 1) + b'\x10'
    third += when(ready, completed)
    fourth = b'\x81\x02\x00\x02\x00\x06'  # Goto frame 3, then play; do not reissue initial loads.
    return swf([first, second, third, fourth])

def main():
    """Write deterministic startup/child SWFs and form data beside this generator."""
    folder = Path(__file__).resolve().parent
    (folder / 'nested').mkdir(exist_ok=True)
    (folder / 'start.swf').write_bytes(root_movie())
    (folder / 'nested/child.swf').write_bytes(child_movie())
    (folder / 'root-vars.txt').write_text('greeting=hello+from+root', encoding='ascii')
    (folder / 'nested/child-vars.txt').write_text('answer=child%20origin%20works', encoding='ascii')


if __name__ == '__main__':
    main()
