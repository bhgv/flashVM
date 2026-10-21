#!/usr/bin/env python3
"""Independent tag-zero limits handshake, sized slots and negotiated media geometry."""
import ctypes as C
import sys
from srecord_oracle import frame, integer, Record
from smedia_oracle import Info, Piece, Profile, Receiver
from sslot_oracle import Report


class Limits(C.Structure):
    _fields_ = [(name, C.c_ulong) for name in ('tag', 'record_limit', 'slot_bytes', 'part_bytes')]


class Boundary(C.Structure):
    _fields_ = [(name,C.c_ulong) for name in ('stream','epoch','object')]


class Message(C.Structure):
    _fields_ = [('command', C.c_ulong), ('id', C.c_ulong), ('limits', Limits), ('boundary', Boundary)]


class Exchange(C.Structure):
    _fields_ = [('struct_size', C.c_size_t), ('ceiling', Limits), ('pending', Limits),
                ('active', Limits), ('last_id', C.c_ulong), ('pending_id', C.c_ulong),
                ('active_id', C.c_ulong), ('role', C.c_ulong),
                ('pending_boundary',Boundary),('active_boundary',Boundary)]


def encoded(message):
    values = [2, message.command, message.id] + [getattr(message.limits, name)
              for name, _ in Limits._fields_] + [0]
    if message.command in (3,4):
        values += [message.boundary.stream,message.boundary.epoch,message.boundary.object]
    return frame(0, b''.join(map(integer, values)))


def main():
    lib = C.CDLL(sys.argv[1])
    write, read = lib.fvm_scontrol_write, lib.fvm_scontrol_read
    init, propose, respond, commit, cancel = (lib.fvm_scontrol_init, lib.fvm_scontrol_propose,
        lib.fvm_scontrol_respond, lib.fvm_scontrol_commit, lib.fvm_scontrol_cancel)
    write.argtypes = [C.POINTER(Message), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    read.argtypes = [C.c_void_p, C.c_size_t, C.POINTER(Message), C.POINTER(C.c_size_t)]
    init.argtypes = [C.POINTER(Exchange), C.POINTER(Limits)]
    propose.argtypes = [C.POINTER(Exchange), C.c_ulong, C.POINTER(Limits), C.POINTER(Message)]
    respond.argtypes = [C.POINTER(Exchange), C.POINTER(Message), C.POINTER(Message)]
    commit.argtypes = [C.POINTER(Exchange), C.POINTER(Message)]
    cancel.argtypes = [C.POINTER(Exchange)]
    maximum, local = Limits(19, 65504, 65536, 65440), Limits(19, 4064, 4096, 4000)
    sender, receiver, offer, ack = Exchange(), Exchange(), Message(), Message()
    assert init(C.byref(sender), C.byref(maximum)) == 0
    assert init(C.byref(receiver), C.byref(local)) == 0
    assert propose(C.byref(sender), 1, C.byref(maximum), C.byref(offer)) == 0
    assert sender.active_id == 0 and sender.pending_id == 1
    out, used, message = C.create_string_buffer(128), C.c_size_t(9999), Message()
    wire = encoded(offer)
    assert write(C.byref(offer), out, 128, C.byref(used)) == 0 and out.raw[:used.value] == wire
    assert len(wire) < 128
    assert read(wire+wire, len(wire)*2, C.byref(message), C.byref(used)) == 0
    assert used.value == len(wire) and bytes(message) == bytes(offer)
    assert respond(C.byref(receiver), C.byref(message), C.byref(ack)) == 0
    assert bytes(ack.limits) == bytes(local) and not receiver.active_id
    saved = bytes(sender)
    forged = Message(2, 1, maximum); forged.limits.slot_bytes += 1024
    assert commit(C.byref(sender), C.byref(forged)) == 2 and bytes(sender) == saved
    forged = Message(2, 1, Limits(20, 65504, 65536, 65440))
    assert commit(C.byref(sender), C.byref(forged)) == 5 and bytes(sender) == saved
    assert commit(C.byref(receiver), C.byref(ack)) == 0
    ack_wire = encoded(ack)
    assert read(ack_wire, len(ack_wire), C.byref(message), C.byref(used)) == 0
    assert commit(C.byref(sender), C.byref(message)) == 0
    assert bytes(sender.active) == bytes(receiver.active) == bytes(local)
    assert commit(C.byref(sender), C.byref(message)) == 0
    # Pending/cancelled negotiations leave active geometry intact and never recycle IDs.
    active = bytes(sender.active)
    assert propose(C.byref(sender), 2, C.byref(local), C.byref(offer)) == 0
    saved, previous = bytes(sender), bytes(offer)
    assert propose(C.byref(sender), 3, C.byref(local), C.byref(offer)) == 5
    assert bytes(sender) == saved and bytes(offer) == previous
    assert cancel(C.byref(sender)) == 0 and bytes(sender.active) == active
    assert commit(C.byref(sender), C.byref(Message(2, 2, local))) == 5
    assert propose(C.byref(sender), 2, C.byref(local), C.byref(offer)) == 5
    # Compact records negotiate only their quota; no forced padding or part size.
    compact = Limits(19, 63, 0, 0)
    assert propose(C.byref(sender), 3, C.byref(compact), C.byref(offer)) == 0
    assert respond(C.byref(receiver), C.byref(offer), C.byref(ack)) == 0
    assert not ack.limits.slot_bytes and not ack.limits.part_bytes
    assert commit(C.byref(receiver), C.byref(ack)) == commit(C.byref(sender), C.byref(ack)) == 0
    assert sender.active.record_limit == 63
    # Unknown tag-zero service versions/commands do not get interpreted as limits/bindings.
    before, prior = bytes(message), used.value
    for payload, status in [(integer(1)+integer(0), 6), (integer(3), 6),
                            (integer(2)+integer(99), 6), (integer(2)+integer(1), 2)]:
        raw = frame(0, payload)
        assert read(raw, len(raw), C.byref(message), C.byref(used)) == status
        assert bytes(message) == before and used.value == prior
    raw = encoded(Message(1, 8, local))
    for cut in range(len(raw)):
        assert read(raw, cut, C.byref(message), C.byref(used)) == 1
        assert bytes(message) == before and used.value == prior
    for values in [[2,1,0,19,4064,4096,4000,0], [2,1,8,0,4064,4096,4000,0],
                   [2,1,8,19,4064,4096,4001,0], [2,1,8,19,4065,4096,4000,0],
                   [2,1,8,19,63,0,1,0], [2,1,8,19,4064,4096,4000,1],
                   [2,1,8,19,4064,4096,4000,0,0], [2,1,8,19,-1,0,0,0]]:
        bad = frame(0,b''.join(map(integer,values)))
        assert read(bad,len(bad),C.byref(message),C.byref(used)) == 2, values
        assert bytes(message)==before and used.value==prior
    # Default writer and scatter writer produce exactly the same bytes as the independent oracle.
    pair = lib.fvm_srecord_write_pair
    pair.argtypes = [C.c_ulong,C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t,
                     C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    assert pair(7,b'abc',3,b'def',3,out,128,C.byref(used))==0
    assert out.raw[:used.value]==frame(7,b'abcdef')
    saved=out.raw; prior=used.value
    assert pair(7,b'a',1,b'b',1,out,1,C.byref(used))==3 and out.raw==saved and used.value==prior
    # Actual configured slot sizes, including maximum: damaged length never crosses the next slot.
    sw,sr=lib.fvm_sslot_write_sized,lib.fvm_sslot_read_sized
    sw.argtypes=[C.c_ulong,C.c_void_p,C.c_size_t,C.c_size_t,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    sr.argtypes=[C.c_void_p,C.c_size_t,C.c_size_t,C.POINTER(Record),C.POINTER(C.c_size_t)]
    record=Record()
    for extent in (256,1024,4096,16384,65536):
        data=b'x'*(extent-64); storage=C.create_string_buffer(extent)
        assert sw(19,data,len(data),extent,storage,extent,C.byref(used))==0 and used.value==extent
        assert storage.raw==frame(19,data).ljust(extent,b'\0')
        assert sr(storage,extent,extent,C.byref(record),C.byref(used))==0
        assert record.size==len(data) and C.string_at(record.payload,record.size)==data
    callback=C.CFUNCTYPE(None,C.c_size_t,C.c_int,C.POINTER(Record),C.c_void_p)
    batch=lib.fvm_sslot_batch_sized
    batch.argtypes=[C.c_void_p,C.c_size_t,C.c_size_t,callback,C.c_void_p,C.POINTER(Report)]
    seen=[]
    @callback
    def observe(index,status,view,userdata):
        seen.append((index,status,bool(view)))
    report=Report()
    for extent in (256,4096,65536):
        good=frame(19,b'x'*(extent-64)).ljust(extent,b'\0')
        raw=good+b'\x7f'*extent+good
        seen.clear()
        assert batch(raw,len(raw),extent,observe,None,C.byref(report))==0
        assert (report.slots,report.valid,report.damaged)==(3,2,1)
        assert seen[0]==(0,0,True) and seen[1][1]!=0 and seen[2]==(2,0,True)
    # Use selected 4096/4000 geometry in the typed media receiver, not just a metadata handshake.
    profile=Profile(local.slot_bytes,local.part_bytes); info=Info(1,7,8,9,120,8017)
    mw,mr,mi=lib.fvm_smedia_write_profile,lib.fvm_smedia_read_profile,lib.fvm_smedia_receiver_init_profile
    mw.argtypes=[C.POINTER(Profile),C.c_ulong,C.POINTER(Info),C.c_ulong,C.c_void_p,C.c_size_t,
                 C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    mr.argtypes=[C.POINTER(Profile),C.c_void_p,C.c_size_t,C.c_ulong,C.POINTER(Piece)]
    mi.argtypes=[C.POINTER(Receiver),C.POINTER(Profile),C.c_ulong,C.POINTER(Info),C.c_void_p,C.c_size_t]
    accept=lib.fvm_smedia_accept
    accept.argtypes=[C.POINTER(Receiver),C.c_void_p,C.c_size_t,C.POINTER(C.c_int)]
    obj=bytes((i*31+7)%256 for i in range(info.total)); destination=C.create_string_buffer(info.total)
    native=Receiver();complete=C.c_int();piece=Piece();storage=C.create_string_buffer(4096)
    assert mi(C.byref(native),C.byref(profile),19,C.byref(info),destination,info.total)==0
    for part in (2,0,1):
        data=obj[part*4000:(part+1)*4000]
        assert mw(C.byref(profile),19,C.byref(info),part,data,len(data),storage,4096,C.byref(used))==0
        values=[1,1,7,8,9,120,8017,part]
        assert storage.raw==frame(19,b''.join(map(integer,values))+data).ljust(4096,b'\0')
        assert mr(C.byref(profile),storage,4096,19,C.byref(piece))==0 and piece.size==len(data)
        assert accept(C.byref(native),storage,4096,C.byref(complete))==0
    assert complete.value and destination.raw==obj
    at = lib.fvm_scontrol_propose_at
    at.argtypes=[C.POINTER(Exchange),C.c_ulong,C.POINTER(Limits),C.POINTER(Boundary),C.POINTER(Message)]
    agreed=lib.fvm_smedia_receiver_init_agreed
    agreed.argtypes=[C.POINTER(Receiver),C.POINTER(Exchange),C.POINTER(Info),C.c_void_p,C.c_size_t]
    previous=None
    for identity, desired in [(4,maximum),(5,Limits(19,480,512,400)),(6,maximum)]:
        boundary=Boundary(7,8,identity+6)
        assert at(C.byref(sender),identity,C.byref(desired),C.byref(boundary),C.byref(offer))==0
        assert offer.command==3
        raw=encoded(offer)
        assert read(raw,len(raw),C.byref(message),C.byref(used))==0
        assert respond(C.byref(receiver),C.byref(message),C.byref(ack))==0 and ack.command==4
        altered=Message();C.memmove(C.byref(altered),C.byref(ack),C.sizeof(ack))
        altered.boundary.object+=1;saved=bytes(sender)
        assert commit(C.byref(sender),C.byref(altered))==5 and bytes(sender)==saved
        raw=encoded(ack)
        assert read(raw,len(raw),C.byref(message),C.byref(used))==0
        assert commit(C.byref(receiver),C.byref(ack))==commit(C.byref(sender),C.byref(message))==0
        profile=Profile(sender.active.slot_bytes,sender.active.part_bytes)
        assert profile.slot_bytes==(512 if identity==5 else 4096)
        info=Info(2,7,8,boundary.object,identity*40,8017)
        assert agreed(C.byref(native),C.byref(sender),C.byref(info),destination,info.total)==0
        prior=bytes(native);info.object-=1
        assert agreed(C.byref(native),C.byref(sender),C.byref(info),destination,info.total)==5
        assert bytes(native)==prior;info.object+=1
        if previous is not None:
            snapshot=destination.raw
            assert accept(C.byref(native),previous,len(previous),C.byref(complete))!=0
            assert bytes(native)==prior and destination.raw==snapshot
        storage=C.create_string_buffer(profile.slot_bytes)
        for part in range((info.total+profile.part_bytes-1)//profile.part_bytes):
            data=obj[part*profile.part_bytes:(part+1)*profile.part_bytes]
            assert mw(C.byref(profile),19,C.byref(info),part,data,len(data),storage,profile.slot_bytes,C.byref(used))==0
            if part==0:previous=storage.raw
            assert accept(C.byref(native),storage,profile.slot_bytes,C.byref(complete))==0
        assert complete.value and destination.raw==obj
    print('PASS tag-zero service negotiation, atomic commit/cancel, compact mode, sized slots actual 4-KiB media parts and live shrink/grow object boundaries')


if __name__=='__main__':main()
