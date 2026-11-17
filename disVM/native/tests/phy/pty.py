# Copyright (c) 2026 bhgv
# EN: Independent original 9P oracle over a raw PTY; no TCP/UDP or socketpair is involved.
# RU: Независимый original 9P oracle через raw PTY, без TCP/UDP/socketpair.
import os
import selectors
import struct
import subprocess
import sys
import tty


# EN/RU: Read exact PTY bytes with a bounded readiness deadline; EOF is explicit failure.
# Читать exact PTY bytes с bounded readiness deadline; EOF означает failure.
def exact(fd, count):
    output = b''
    with selectors.DefaultSelector() as ready:
        ready.register(fd, selectors.EVENT_READ)
        while len(output) < count:
            if not ready.select(5):
                raise RuntimeError('PTY read deadline')
            data = os.read(fd, count-len(output))
            if not data:
                raise RuntimeError('PTY EOF')
            output += data
    return output


# EN/RU: Encode one counted original 9P string, independent of production C codecs.
# Кодировать counted original 9P string независимо от C codecs.
def string(value):
    return struct.pack('<H', len(value))+value


# EN/RU: Drive exact native transactions, fragment every response, close and check worker retirement.
# Выполнить exact native transactions с fragmented responses, закрыть и проверить retirement.
def main():
    master, slave = os.openpty()
    tty.setraw(slave)
    mode = sys.argv[2]
    child = subprocess.Popen([sys.argv[1], str(slave), mode], pass_fds=(slave,),
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        qid = struct.pack('<BIQ', 0, 0, 42)
        if mode == 'reject':
            output, failure = child.communicate(timeout=5)
            if child.returncode or 'PASS unreliable PHY rejected before 9P wire effects' not in output:
                raise RuntimeError(str(child.returncode)+output+failure)
            with selectors.DefaultSelector() as ready:
                ready.register(master, selectors.EVENT_READ)
                if ready.select(0):
                    raise RuntimeError('rejected PHY emitted bytes')
            print('PASS unreliable PHY refusal without wire effects')
            return
        kinds = (100, 104, 110, 112, 116) if mode == 'prefix-error' else (100, 104, 110, 112, 116, 120)
        for kind in kinds:
            size = struct.unpack('<I', exact(master, 4))[0]
            if not 7 <= size <= 256:
                raise RuntimeError('invalid native frame size')
            request = exact(master, size-4)
            actual, tag = struct.unpack_from('<BH', request)
            if actual != kind:
                raise RuntimeError(f'unexpected request {actual} expected {kind}')
            if kind == 100:
                payload = struct.pack('<I', 256)+string(b'9P2000')
            elif kind == 104:
                payload = qid
            elif kind == 110:
                payload = b'\0\0'
            elif kind == 112:
                payload = qid+struct.pack('<I', 0)
            elif kind == 116:
                payload = struct.pack('<I', 6)+b'serial'
            else:
                payload = b''
            response = struct.pack('<IBH', 7+len(payload), kind+1, tag)+payload
            for offset in range(0, len(response), 2):
                count = os.write(master, response[offset:offset+2])
                if count != len(response[offset:offset+2]):
                    raise RuntimeError('PTY oracle short write')
        output, failure = child.communicate(timeout=5)
        if child.returncode or 'PASS real native Styx transactions over nonsocket PTY PHY' not in output:
            raise RuntimeError(str(child.returncode)+output+failure)
        print('PASS independent 9P/PTY framing without sockets: '+mode)
    finally:
        if child.poll() is None:
            child.kill()
            child.communicate(timeout=5)
        os.close(slave)
        os.close(master)


if __name__ == '__main__':
    main()
