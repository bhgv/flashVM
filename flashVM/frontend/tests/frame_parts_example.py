#!/usr/bin/env python3
"""Independent all-pixel checks of real frame parts and native reassembly over actual TCP/UDP."""
import ctypes as C
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tests'/'protocol'))
from srecord_oracle import frame as encode, integer as encode_integer
from smedia_oracle import Info, Profile, Receiver
from smeasure_oracle import MediaMeasure
from sadapt_oracle import Feedback


def integer(data, offset=0):
    start = offset
    value = data[offset] & 63
    assert not data[offset] & 64
    while not data[offset] & 128:
        offset += 1
        value = value*128+(data[offset] & 127)
    offset += 1
    assert data[start:offset] == encode_integer(value)
    return value, offset


def fields(data, count):
    result, offset = [], 0
    for _ in range(count):
        value, offset = integer(data, offset)
        result.append(value)
    return result, data[offset:]


def record(data, offset=0):
    start = offset
    _, offset = integer(data, offset)
    length, offset = integer(data, offset)
    end = offset+length
    assert end <= len(data)
    tag, offset = integer(data, offset)
    payload = data[offset:end]
    assert encode(tag, payload) == data[start:end]  # Independent zlib checksum and canonical fields.
    return tag, payload, end


def expected(sequence):
    left, color = (4, (64,128,255,255)) if sequence & 1 else (18, (255,128,64,255))
    return bytes(v for y in range(32) for x in range(32)
                 for v in (color if left <= x < left+8 and 4 <= y < 12 else (255,255,255,255)))


def parse(data, mode, adaptive=False, feedback_reports=None):
    tag, declaration, offset = record(data)
    assert tag == 0
    values, names = fields(declaration, 2)
    assert values == [1,2]
    for expected_tag, expected_name in [(19,b'flashvm.media.fragment'), (20,b'flashvm.player.frame.metadata')]:
        values, names = fields(names,3)
        tag, version, count = values
        assert tag == expected_tag and version == 1 and names[:count] == expected_name
        names = names[count:]
    assert not names
    objects = []
    profiles = [(1,1024,768), (2,512,400), (3,1024,768), (4,4096,4000)] if adaptive else [(1,1024,768), (2,512,400), (3,4096,4000)]
    for sequence, slot_bytes, part_bytes in profiles:
        if adaptive and sequence > 1:
            tag, payload, offset = record(data,offset)
            values, tail = fields(payload,11)
            assert tag == 0 and not tail and values == (feedback_reports or reports())[sequence-2]

        tag, metadata, offset = record(data,offset)
        values, source = fields(metadata,17)
        assert tag == 20 and values[:1] == [1] and values[1] and values[2]
        assert values[3:9] == [sequence,1,32,32,128,4096]
        assert values[9:14] == [sequence,sequence,sequence,sequence,1]
        assert values[14:16] == [0,0] and len(source) == values[16] and source.endswith(b'frame-parts.swf')
        for command in (3,4):
            tag, payload, offset = record(data,offset)
            values, tail = fields(payload,11)
            assert tag == 0 and not tail
            assert values == [2,command,sequence,19,slot_bytes-32,slot_bytes,part_bytes,0,7,1,sequence]
        slots, image = [], b''
        count = (4096+part_bytes-1)//part_bytes
        for part in range(count):
            slot = data[offset:offset+slot_bytes]
            assert len(slot) == slot_bytes
            tag, payload, end = record(slot)
            values, raw = fields(payload,8)
            assert tag == 19 and not any(slot[end:])
            assert values == [1,mode,7,1,sequence,sequence*40,4096,part]
            assert len(raw) == min(part_bytes,4096-part*part_bytes)
            slots.append(slot); image += raw; offset += slot_bytes
        assert image == expected(sequence)
        objects.append((sequence,Profile(slot_bytes,part_bytes),slots))
    assert offset == len(data)
    return objects


def transport(lib, objects, mode):
    init, accept, missing = lib.fvm_smedia_receiver_init_profile,lib.fvm_smedia_accept,lib.fvm_smedia_missing
    init.argtypes = [C.POINTER(Receiver),C.POINTER(Profile),C.c_ulong,C.POINTER(Info),C.c_void_p,C.c_size_t]
    accept.argtypes = [C.POINTER(Receiver),C.c_void_p,C.c_size_t,C.POINTER(C.c_int)]
    missing.argtypes = [C.POINTER(Receiver),C.c_ulong,C.POINTER(C.c_ulong),C.c_size_t,C.POINTER(C.c_size_t)]
    for sequence, profile, slots in objects:
        info = Info(mode,7,1,sequence,sequence*40,4096)
        state, complete = Receiver(),C.c_int(77)
        image = C.create_string_buffer(b'\xa5'*4096,4096)
        assert init(C.byref(state),C.byref(profile),19,C.byref(info),image,4096) == 0
        # The loopback path supports these explicit test datagram sizes; no Internet MTU claim.
        with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as rx, socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as tx:
            rx.settimeout(3);rx.bind(('127.0.0.1',0))
            for part in reversed(range(len(slots)-1)):  # Deliberately omit the final part.
                raw = b'\x7f'*profile.slot_bytes if part == 0 else slots[part]
                tx.sendto(raw,rx.getsockname());received,_ = rx.recvfrom(profile.slot_bytes+1)
                result = accept(C.byref(state),received,len(received),C.byref(complete))
                assert (result != 0) == (part == 0)
                if result == 0:
                    assert accept(C.byref(state),received,len(received),C.byref(complete)) == 0  # Duplicate.
        assert state.received == len(slots)-2
        holes = (C.c_ulong*4096)(); count = C.c_size_t(9999)
        assert missing(C.byref(state),0,holes,4096,C.byref(count)) == 0
        if mode == 2:
            assert count.value == 0  # No repair request for immediate-media losses.
        else:
            assert list(holes[:count.value]) == [0,len(slots)-1]
            # Compact control requests and only selected parts travel on reliable TCP.
            request = encode(41,b''.join(map(encode_integer,[7,1,sequence,*holes[:count.value]])))
            with socket.socket() as listener:
                listener.settimeout(3);listener.bind(('127.0.0.1',0));listener.listen(1)
                with socket.create_connection(listener.getsockname(),timeout=3) as client:
                    server,_ = listener.accept()
                    with server:
                        server.settimeout(3);client.sendall(request)
                        wire=b''
                        while len(wire)<len(request):wire += server.recv(37)
                        assert wire == request
                        for part in holes[:count.value]:
                            server.sendall(slots[part]);wire=b''
                            while len(wire)<profile.slot_bytes:wire += client.recv(83)
                            assert accept(C.byref(state),wire,len(wire),C.byref(complete)) == 0
            assert complete.value == 1 and image.raw == expected(sequence)
        # A full valid delivery works in both modes, independently of the deliberate loss case.
        assert init(C.byref(state),C.byref(profile),19,C.byref(info),image,4096) == 0
        for raw in reversed(slots):
            assert accept(C.byref(state),raw,len(raw),C.byref(complete)) == 0
        assert complete.value == 1 and image.raw == expected(sequence)


def measure_link(lib, objects, mode):
    """Measure actual exported slots on UDP, encode host feedback, then drive another player run."""
    init = lib.fvm_smedia_receiver_init_profile
    init.argtypes = [C.POINTER(Receiver),C.POINTER(Profile),C.c_ulong,C.POINTER(Info),C.c_void_p,C.c_size_t]
    begin, accept, finish = lib.fvm_smeasure_media_init,lib.fvm_smeasure_media_accept,lib.fvm_smeasure_media_finish
    begin.argtypes = [C.POINTER(MediaMeasure),C.POINTER(Receiver),C.c_ulong,C.c_ulong]
    accept.argtypes = [C.POINTER(MediaMeasure),C.c_void_p,C.c_size_t,C.c_ulong,C.POINTER(C.c_int)]
    finish.argtypes = [C.POINTER(MediaMeasure),*([C.c_ulong]*3),C.POINTER(Feedback)]
    write = lib.fvm_sadapt_feedback_write
    write.argtypes = [C.POINTER(Feedback),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    windows, wire = [],b''
    for sequence,profile,slots in objects[:3]:
        info = Info(mode,7,1,sequence,sequence*40,4096)
        receiver, observation, complete = Receiver(),MediaMeasure(),C.c_int(77)
        image = C.create_string_buffer(b'\xa5'*4096,4096)
        assert init(C.byref(receiver),C.byref(profile),19,C.byref(info),image,4096) == 0
        # Relative elapsed time comes from this host's monotonic clock, not packet metadata.
        deadline = 100
        assert begin(C.byref(observation),C.byref(receiver),sequence,deadline) == 0
        start = time.monotonic()
        with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as rx, socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as tx:
            rx.settimeout(3);rx.bind(('127.0.0.1',0))
            selected = range(len(slots)-1) if sequence == 1 else range(len(slots))
            for part in reversed(list(selected)):
                raw = b'\x7f'*profile.slot_bytes if sequence == 1 and part == 0 else slots[part]
                tx.sendto(raw,rx.getsockname());packet,_ = rx.recvfrom(profile.slot_bytes+1)
                elapsed = int((time.monotonic()-start)*1000)
                before = bytes(observation),bytes(receiver),image.raw
                result = accept(C.byref(observation),packet,len(packet),elapsed,C.byref(complete))
                if sequence == 1 and part == 0:
                    assert result != 0 and before == (bytes(observation),bytes(receiver),image.raw)
                else:
                    assert result == 0
                    assert accept(C.byref(observation),packet,len(packet),elapsed,C.byref(complete)) == 0
        # A real TCP request/echo supplies RTT, independently of the media presentation timestamp.
        with socket.socket() as listener:
            listener.settimeout(3);listener.bind(('127.0.0.1',0));listener.listen(1)
            with socket.create_connection(listener.getsockname(),timeout=3) as client:
                server,_ = listener.accept()
                with server:
                    server.settimeout(3);probe = time.monotonic();client.sendall(b'probe')
                    data = b''
                    while len(data)<5:data += server.recv(5-len(data))
                    server.sendall(data);data = b''
                    while len(data)<5:data += client.recv(5-len(data))
                    rtt = int((time.monotonic()-probe)*1000)
                    assert data == b'probe'
        remaining = deadline/1000-(time.monotonic()-start)
        if remaining>0:time.sleep(remaining)  # Close at the actual host deadline, never fabricate elapsed time.
        elapsed = max(deadline,int((time.monotonic()-start)*1000))
        feedback = Feedback()
        assert finish(C.byref(observation),elapsed,1,rtt,C.byref(feedback)) == 0
        values = [3,1]+[getattr(feedback,name) for name,_ in Feedback._fields_]
        expected_count = len(slots)-2 if sequence == 1 else len(slots)
        assert feedback.expected == len(slots) and feedback.on_time == expected_count
        assert feedback.late == feedback.damaged == 0  # A corrupt unidentified datagram remains missing.
        output = C.create_string_buffer(128);used = C.c_size_t()
        assert write(C.byref(feedback),output,128,C.byref(used)) == 0
        encoded = output.raw[:used.value]
        assert encoded == trace([values])
        windows.append(values);wire += encoded
        before = bytes(observation); complete.value = 77
        assert accept(C.byref(observation),slots[-1],len(slots[-1]),elapsed,C.byref(complete)) == 5
        assert bytes(observation) == before and complete.value == 77
    return windows,wire


def reports():
    # Controlled host windows: six normal pieces with two unusable, then clean smaller/normal images.
    return [[3,1,7,1,1,6,4,0,1,1,20], [3,1,7,1,2,11,11,0,0,1,20],
            [3,1,7,1,3,6,6,0,0,1,20]]


def trace(windows):
    return b''.join(encode(0,b''.join(map(encode_integer,window))) for window in windows)


def main():
    executable = str(Path(sys.argv[1]).resolve()); lib = C.CDLL(sys.argv[2])
    env = dict(os.environ,SDL_VIDEODRIVER='flashvm-driver-that-does-not-exist',SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory(prefix='frame-parts-') as temporary:
        output = Path(temporary)/'frames.sparts'
        for mode in (1,2):
            result = subprocess.run([executable,*(['--live'] if mode==2 else []),str(output)],env=env,
                                    capture_output=True,text=True,timeout=8)
            assert result.returncode == 0,(result.stdout,result.stderr)
            assert result.stdout.strip() == 'exported_frames=3'
            transport(lib,parse(output.read_bytes(),mode),mode)
        feedback = Path(temporary)/'feedback.srecords'
        generator = Path(__file__).resolve().parents[2]/'examples/player/frame_parts/make_feedback.py'
        result = subprocess.run([sys.executable,str(generator),str(feedback)],capture_output=True,timeout=8)
        assert result.returncode == 0 and feedback.read_bytes() == trace(reports())
        for mode in (1,2):
            result = subprocess.run([executable,'--feedback',str(feedback),*(['--live'] if mode==2 else []),str(output)],
                                    env=env,capture_output=True,text=True,timeout=8)
            assert result.returncode == 0,(result.stdout,result.stderr)
            assert result.stdout.strip() == 'exported_frames=4'
            objects = parse(output.read_bytes(),mode,True)
            transport(lib,objects,mode)
            measured, encoded = measure_link(lib,objects,mode)
            measured_path = Path(temporary)/'measured.srecords';measured_path.write_bytes(encoded)
            result = subprocess.run([executable,'--feedback',str(measured_path),*(['--live'] if mode==2 else []),str(output)],
                                    env=env,capture_output=True,text=True,timeout=8)
            assert result.returncode == 0 and result.stdout.strip() == 'exported_frames=4',(result.stdout,result.stderr)
            transport(lib,parse(output.read_bytes(),mode,True,measured),mode)
        valid = feedback.read_bytes()
        for bad in (b'',b'\x80'*385,valid[:-1],valid+b'\x80',valid+valid,valid[:8]+b'\x00'+valid[9:]):
            feedback.write_bytes(bad);output.write_bytes(b'preserve destination')
            result = subprocess.run([executable,'--feedback',str(feedback),str(output)],env=env,capture_output=True,timeout=8)
            assert result.returncode == 1 and output.read_bytes() == b'preserve destination'
        for field,value in ((2,8),(3,2),(4,9),(5,7)):
            windows = reports();windows[0][field] = value
            feedback.write_bytes(trace(windows));output.unlink(missing_ok=True)
            result = subprocess.run([executable,'--feedback',str(feedback),str(output)],env=env,capture_output=True,timeout=8)
            assert result.returncode == 1 and not output.exists()  # Causal count is checked during export.
        feedback.write_bytes(valid)
        for args in (['--feedback'],['--feedback',str(feedback)],['--feedback',str(feedback),str(feedback)],
                     ['--feedback',str(feedback),'--feedback',str(feedback),str(output)],['--live','--live',str(output)]):
            result = subprocess.run([executable,*args],env=env,capture_output=True,timeout=8)
            assert result.returncode == 2
        for args,status in [([],2),(['--unknown'],2),(['--live'],2),(['--help'],0),([temporary],1),
                            (['--live','--unknown'],2),([str(Path(temporary)/'missing'/'out')],1)]:
            result = subprocess.run([executable,*args],env=env,capture_output=True,text=True,timeout=8)
            assert result.returncode == status,(args,result.stdout,result.stderr)
    print('PASS real RGBA frame parts, compact metadata, scripted and feedback-driven shrink/grow, post-SDK retention and UDP/TCP file/live loss policies')


if __name__ == '__main__':main()
