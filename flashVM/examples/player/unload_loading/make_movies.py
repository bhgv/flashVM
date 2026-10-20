#!/usr/bin/env python3
"""Generate the commented unload SWF8 tutorial using only Python standard-library bytes."""
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

def require_undefined(path):
    """Throw when obsolete content or a cancelled response remains visible."""
    failure = push('stale content: ' + path) + b'\x2a'
    return get(path) + UNDEFINED + b'\x66\x9d\x02\0' + struct.pack('<h', len(failure)) + failure


def movie(network=True, object_form=False):
    """Cancel old subtree loads, retain the instance and verify later reloads."""
    first = method_call('_root', 'createEmptyMovieClip', 'child', 1)
    first += method_call('_root.child', 'createEmptyMovieClip', 'grand', 1)
    first += method_call('_root', 'createEmptyMovieClip', 'peer', 2)
    first += push('_root.child.kept', 42) + b'\x1d'
    first += push('_root.saved') + get('_root.child') + b'\x1d'
    first += push('_root.iter', 0) + b'\x1d'
    if network:
        first += method_call('_root.child', 'loadVariables', 'slow.txt')
        first += method_call('_root.child.grand', 'loadVariables', 'never.txt')
        first += method_call('_root.peer', 'loadVariables', 'peer.txt')
    second = b''
    if object_form:
        second += get('_root.child') + push(1, 'unloadMovie') + b'\x3d\x17'
    else:
        second += global_call('unloadMovie', 'child')
    if network:
        # This request follows the unload and must survive the earlier queue cancellations.
        second += method_call('_root.child', 'loadVariables', 'fresh.txt')
    checks = require('_root.child.kept', 42) + require('_root.child._totalframes', 1)
    checks += require_undefined('_root.child.grand') + require_undefined('_root.child.poison')
    failure = push('instance identity changed') + b'\x2a'
    checks += get('_root.saved') + get('_root.child') + b'\x66\x9d\x02\0' + struct.pack('<h', len(failure)) + failure
    if network:
        checks += require('_root.peer.answer', 'peer') + require('_root.child.answer', 'fresh')
    checks += method_call('_root.child', 'unloadMovie') + b'\x07'
    if not network:
        return swf([first, second, checks])
    third = push('_root.iter') + get('_root.iter') + push(1) + b'\x47\x1d'
    third += when(get('_root.iter') + push(250) + b'\x67', require('_root.finished', 1))
    third += when(equal('_root.child.answer', 'fresh'), checks + push('_root.finished', 1) + b'\x1d')
    fourth = b'\x81\x02\x00\x02\x00\x06'
    return swf([first, second, third, fourth])


def main():
    """Write reproducible bytecode and form files without a compiler or test imports."""
    folder = Path(__file__).resolve().parent
    (folder / 'start.swf').write_bytes(movie())
    for name, body in {'slow.txt': 'poison=1', 'never.txt': 'poison=1',
                       'peer.txt': 'answer=peer', 'fresh.txt': 'answer=fresh'}.items():
        (folder / name).write_text(body, encoding='ascii')


if __name__ == '__main__':
    main()
