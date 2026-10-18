"""Exercise the optional C89 runner with generated two-frame FWS/CWS inputs."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib

runner = str(pathlib.Path(sys.argv[1]).resolve())
def tag(code, data=b''):
    return struct.pack('<H', code << 6 | len(data)) + data

def text(value):
    payload = b'\0' + value.encode('ascii') + b'\0'
    return b'\x96' + struct.pack('<H', len(payload)) + payload

def actions(label):
    return text('this') + b'\x1c' + text('_root') + b'\x1c\x66\x26' + text(label) + b'\x26\0'

def movie(tags):
    body = b'\x08\0' + struct.pack('<HH', 12*256, 2) + tags
    return b'FWS\x08' + struct.pack('<I', len(body)+8) + body

def run(args, code, output=None):
    result = subprocess.run([runner] + args, capture_output=True, timeout=15)
    assert result.returncode == code, (args, result.returncode, result.stderr)
    if output is not None:
        assert result.stdout == output, (args, result.stdout)
    if code:
        assert result.stderr
    else:
        assert not result.stderr

with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    raw = movie(tag(9, b'\x12\x34\x56') + tag(12, actions('one')) + tag(1) +
                tag(12, actions('two')) + tag(1) + tag(0))
    for name, data in [('fws', raw), ('cws', b'C'+raw[1:8]+zlib.compress(raw[8:]))]:
        path = root / (name+'.swf')
        path.write_bytes(data)
        run([str(path)], 0, b'false\none\nfalse\ntwo\n')
        run(['--timeline', '1', str(path)], 0, b'true\none\n')
        run(['--timeline', '2', str(path)], 0, b'true\none\ntrue\ntwo\n')
        for ticks in ['0', '-1', '+1', '10001', '999999999999999999', '1x', '']:
            run(['--timeline', ticks, str(path)], 2, b'')
    bad = root / 'bad.swf'
    bad.write_bytes(raw[:-1])
    run(['--timeline', '1', str(bad)], 1, b'')
    bad.write_bytes(movie(tag(83) + tag(1) + tag(1) + tag(0)))
    run(['--timeline', '1', str(bad)], 1, b'')
    run([], 2, b'')

with tempfile.TemporaryDirectory() as directory:
    path = pathlib.Path(directory) / 'command.swf'
    def command(url, flags=None):
        if flags is None:
            payload = url.encode('ascii') + b'\0\0'
            request = b'\x83' + struct.pack('<H', len(payload)) + payload
        else:
            request = text(url) + text('') + b'\x9a\x01\0' + bytes([flags])
        code = text('before') + b'\x26' + request + text('after') + b'\x26\0'
        return movie(tag(12, code) + tag(1) + tag(1) + tag(0))
    path.write_bytes(command('FSCommand:quit'))
    run([str(path)], 1, b'before\n')
    run(['--acknowledge-quit', str(path)], 0, b'before\nafter\n')
    run(['--acknowledge-quit', '--timeline', '1', str(path)], 0, b'before\nafter\n')
    path.write_bytes(command('FSCOMMAND:QUIT', 0))
    run(['--acknowledge-quit', str(path)], 0, b'before\nafter\n')
    for url, flags in [('fscommand:quitX', None), ('https://example.invalid/', None),
                       ('fscommand:quit', 1)]:
        path.write_bytes(command(url, flags))
        run(['--acknowledge-quit', str(path)], 1, b'before\n')
    run(['--acknowledge-quit'], 2, b'')
