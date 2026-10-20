#!/usr/bin/env python3
"""Generate the standalone PNG/GIF/JPEG child loading tutorial using only Python standard-library bytes."""
from pathlib import Path
import struct
import zlib

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

JPEG = b'\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00\xff\xdb\x00C\x00\x02\x01\x01\x01\x01\x01\x02\x01\x01\x01\x02\x02\x02\x02\x02\x04\x03\x02\x02\x02\x02\x05\x04\x04\x03\x04\x06\x05\x06\x06\x06\x05\x06\x06\x06\x07\t\x08\x06\x07\t\x07\x06\x06\x08\x0b\x08\t\n\n\n\n\n\x06\x08\x0b\x0c\x0b\n\x0c\t\n\n\n\xff\xdb\x00C\x01\x02\x02\x02\x02\x02\x02\x05\x03\x03\x05\n\x07\x06\x07\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\xff\xc0\x00\x11\x08\x00\x08\x00\x08\x03\x01"\x00\x02\x11\x01\x03\x11\x01\xff\xc4\x00\x1f\x00\x00\x01\x05\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\xff\xc4\x00\xb5\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05\x05\x04\x04\x00\x00\x01}\x01\x02\x03\x00\x04\x11\x05\x12!1A\x06\x13Qa\x07"q\x142\x81\x91\xa1\x08#B\xb1\xc1\x15R\xd1\xf0$3br\x82\t\n\x16\x17\x18\x19\x1a%&\'()*456789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz\x83\x84\x85\x86\x87\x88\x89\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xff\xc4\x00\x1f\x01\x00\x03\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\xff\xc4\x00\xb5\x11\x00\x02\x01\x02\x04\x04\x03\x04\x07\x05\x04\x04\x00\x01\x02w\x00\x01\x02\x03\x11\x04\x05!1\x06\x12AQ\x07aq\x13"2\x81\x08\x14B\x91\xa1\xb1\xc1\t#3R\xf0\x15br\xd1\n\x16$4\xe1%\xf1\x17\x18\x19\x1a&\'()*56789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xff\xda\x00\x0c\x03\x01\x00\x02\x11\x03\x11\x00?\x00\xfc\x9f\xa2\x8a+\xe2\xcf\xd1\x0f\xff\xd9'
GIF = b'GIF89a\x01\x00\x01\x00\x80\x00\x00\x00\x00\x00\xff\x00\x00!\xf9\x04\x01\x00\x00\x00\x00,\x00\x00\x00\x00\x01\x00\x01\x00\x00\x02\x02D\x01\x00;'

def png():
    """Encode a translucent 8x8 PNG using only stdlib zlib."""
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    row = b'\0' + bytes([64,32,16,128])*8
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',8,8,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(row*8))+chunk(b'IEND',b'')


def require_undefined(path):
    """Throw when an obsolete child binding is still visible."""
    failure=push('stale child: '+path)+b'\x2a'
    return get(path)+UNDEFINED+b'\x66\x9d\x02\0'+struct.pack('<h',len(failure))+failure

def content_query(owner, method):
    """Invoke a zero-argument MovieClip query through ordinary ActionCallMethod."""
    return push(0,owner)+b'\x1c'+push(method)+b'\x52'


def require_content(owner, method, expected):
    """Require exact completed byte/version metadata in an actual SWF frame."""
    failed=push('content query mismatch: '+method)+b'\x2a'
    return content_query(owner,method)+push(expected)+b'\x66\x9d\x02\0'+struct.pack('<h',len(failed))+failed


def dimension(index):
    return push('_root.picture', index) + b'\x22'


def named_dimension(index):
    """GetMember uses the same intrinsic geometry as numeric ActionGetProperty."""
    return get('_root.picture') + push('_width' if index == 8 else '_height') + b'\x4e'


def require_named_dimension(index, expected):
    """Require an exact dimension read through the ordinary property descriptor."""
    failed = push('named dimension mismatch') + b'\x2a'
    return named_dimension(index) + push(expected) + b'\x66\x9d\x02\0' + struct.pack('<h',len(failed)) + failed


def assign_member(name, value):
    """Assign through SetMember, retaining an explicit MovieClip receiver."""
    return get('_root.picture') + push(name,value) + b'\x4f'


def require_dimension(index, expected):
    failed = push('image dimension mismatch') + b'\x2a'
    return dimension(index) + push(expected) + b'\x66\x9d\x02\0' + struct.pack('<h',len(failed)) + failed


def root(kind=0, image_file="picture.png"):
    """Wait for an image, unload it and verify SWF replacement on the same instance."""
    width = 1 if kind == 1 else 8
    payload_size = len([png(),GIF,JPEG][kind])
    first = method_call('_root','createEmptyMovieClip','picture',1)
    first += method_call('_root.picture','createEmptyMovieClip','old',1)
    first += push('_root.picture.kept',42)+b'\x1d'+push('_root.saved')+get('_root.picture')+b'\x1d'
    first += push('_root.iter',0)+b'\x1d'+push('_root.phase',0)+b'\x1d'
    first += require_content('_root','getSWFVersion',8)
    first += get('_root.picture')+push(image_file,2,'loadMovie')+b'\x3d\x17'
    ready = require('_root.picture.kept',42)+require_dimension(9,width)
    ready += require_content('_root.picture','getBytesLoaded',payload_size)
    ready += require_content('_root.picture','getBytesTotal',payload_size)
    ready += require_content('_root.picture','getSWFVersion',-1)
    ready += require_named_dimension(8,width) + require_named_dimension(9,width)
    ready += assign_member('_width',width*2) + assign_member('_height',width*3)
    ready += require_dimension(8,width*2) + require_dimension(9,width*3)
    ready += require_named_dimension(8,width*2) + require_named_dimension(9,width*3)
    ready += push('_root.imageOrigin')+get('_root.picture')+push('_url')+b'\x4e\x1d'
    ready += assign_member('_url','ignored source')
    failed_url = push('readonly image origin changed')+b'\x2a'
    ready += get('_root.imageOrigin')+get('_root.picture')+push('_url')+b'\x4e\x66\x9d\x02\0'+struct.pack('<h',len(failed_url))+failed_url
    ready += require_undefined('_root.picture.old')
    failed=push('image target changed')+b'\x2a'
    ready += get('_root.saved')+get('_root.picture')+b'\x66\x9d\x02\0'+struct.pack('<h',len(failed))+failed
    ready += push('_root.phase',1)+b'\x1d'+method_call('_root.picture','unloadMovie')
    ready += global_call('loadMovie','after.swf','picture')
    # Queued unload/reload has not published new content during this same source frame.
    ready += require_content('_root.picture','getBytesTotal',payload_size)
    second = push('_root.iter')+get('_root.iter')+push(1)+b'\x47\x1d'
    second += when(get('_root.iter')+push(200)+b'\x67',require('_root.finished',1))
    second += when(equal('_root.phase',0)+(dimension(8)+push(width)+b'\x66')+b'\x10',ready)
    # The replacement SWF has run its own frame and therefore owns the published record.
    completed = require('_root.picture.kept', 42)
    completed += require_dimension(8, 0) + require_named_dimension(8, 0)
    completed += require_content('_root.picture', 'getSWFVersion', 8)
    completed += require_content('_root.picture', 'getBytesLoaded', len(after()))
    completed += require_content('_root.picture', 'getBytesTotal', len(after()))
    completed += push('_root.finished', 1) + b'\x1d\x07'
    second += when(equal('_root.picture.loaded', 1), completed)
    return swf([first,second,b'\x81\x02\0\x01\0\x06'])


def after():
    return swf([push('this')+b'\x1c'+push('loaded',1)+b'\x4f\x07'])

def main():
    """Emit reproducible images, startup SWFs and an annotated linked-memory payload."""
    folder=Path(__file__).resolve().parent
    for kind,name in enumerate(['picture.png','picture.gif','picture.jpg']):
        (folder/name).write_bytes([png(),GIF,JPEG][kind])
        (folder/(['png-start.swf','gif-start.swf','jpeg-start.swf'][kind])).write_bytes(root(kind,name))
    (folder/'after.swf').write_bytes(after())
    data=root(0,'picture.png')
    lines=['/* Generated linked startup SWF; regenerate with make_movies.py. */',
           'static const unsigned char embedded_movie[] = { /* Counted immutable SWF bytes. */']
    for offset in range(0,len(data),12):
        lines.append('    '+', '.join('0x%02x'%b for b in data[offset:offset+12])+', /* Binary payload; NUL is ordinary data. */')
    lines.append('}; /* sizeof(embedded_movie) is the exact byte count. */')
    (folder/'movie.inc').write_text('\n'.join(lines)+'\n')

if __name__=='__main__':main()
