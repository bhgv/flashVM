#!/usr/bin/env python3
"""Independent canonical S integers for frame metadata, native queue replies and leased pixels."""
import ctypes as C
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"tests"/"protocol"))
from srecord_oracle import integer


class Handle(C.Structure):
    _fields_=[("id",C.c_ulong),("generation",C.c_ulong)]


class Info(C.Structure):
    _fields_=[("lease",Handle)]+[(n,C.c_ulong) for n in ["sequence","format","width","height",
        "row_bytes","byte_count","movie_ticks","total_ticks","presentations",
        "total_presentations","reason","flags","dropped"]]+[("source",C.c_char*256)]


class Frame(C.Structure):
    _fields_=[("struct_size",C.c_ulong)]+[(n,C.c_uint) for n in ["format","reason","width","height"]]+[
        ("stride",C.c_size_t),("byte_count",C.c_size_t),("pixels",C.c_void_p),("source",C.c_char_p)]+[
        (n,C.c_ulong) for n in ["movie_ticks","total_ticks","presentations","total_presentations"]]


class Request(C.Structure):
    _fields_=[("session",C.c_ulong),("id",C.c_ulong),("target",Handle),("operation",C.c_ulong),
        ("data",C.c_void_p),("size",C.c_size_t)]


class Reply(C.Structure):
    _fields_=[("result",C.c_int),("size",C.c_size_t),("data",C.c_ubyte*512)]


def payload(fields,source=b"memory://pixels"):
    return b"".join(map(integer,fields+[len(source)]))+source


def main():
    lib=C.CDLL(sys.argv[1]);read=lib.fvm_desktop_resource_frame_read
    read.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Info)]
    fields=[1,0,0,1,1,2,2,8,16,0,0,1,1,2,0,0]
    cases=[(fields,b"memory://pixels"),([1,8,0xffffffff,0xffffffff,1,4096,4096,16384,67108864,
        0xffffffff,0xffffffff,0xffffffff,0xffffffff,1,3,0xffffffff],b"x"*255)]
    for values,source in cases:
        data=payload(values,source);out=Info()
        assert read(data,len(data),C.byref(out))==0
        assert out.lease.id==values[1] and out.lease.generation==values[2]
        assert out.sequence==values[3] and out.byte_count==values[8] and out.dropped==values[15]
        assert out.source==source
        before=bytes(out)
        for cut in range(len(data)):
            assert read(data,cut,C.byref(out))==1 and bytes(out)==before
    for index,value in [(0,2),(1,9),(1,1),(2,1),(3,0),(3,1<<32),(4,2),(5,0),(5,4097),(6,0),
                        (7,9),(8,15),(11,0),(12,0),(13,0),(13,3),(14,4),(14,1),(15,-1)]:
        bad=list(fields);bad[index]=value;data=payload(bad);before=bytes(out)
        assert read(data,len(data),C.byref(out))==1 and bytes(out)==before,(index,value)
    for data in [payload(fields)+b"\x80",payload(fields,b"a\0b"),b"\x00\x81"+payload(fields)[1:]]:
        assert read(data,len(data),C.byref(out))==1
    lib.fvm_rdispatch_create.argtypes=[C.c_void_p,C.POINTER(C.c_void_p)]
    lib.fvm_desktop_resource_frame_create.argtypes=[C.c_void_p,C.c_char_p,C.c_size_t,C.POINTER(C.c_void_p),C.POINTER(Handle)]
    lib.fvm_rdispatch_grant.argtypes=[C.c_void_p,Handle,C.c_ulong,C.c_uint]
    lib.fvm_desktop_resource_frame_capture.argtypes=[C.c_void_p,C.POINTER(Frame)]
    lib.fvm_rdispatch_post.argtypes=[C.c_void_p,C.POINTER(Request)]
    lib.fvm_rdispatch_pump.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    lib.fvm_rdispatch_take.argtypes=[C.c_void_p,C.c_ulong,C.c_ulong,C.POINTER(Reply)]
    lib.fvm_desktop_resource_frame_destroy.argtypes=[C.POINTER(C.c_void_p)]
    lib.fvm_rdispatch_destroy.argtypes=[C.POINTER(C.c_void_p)]
    dispatch,adapter=C.c_void_p(),C.c_void_p();target=Handle();used=C.c_size_t();reply=Reply()
    assert lib.fvm_rdispatch_create(None,C.byref(dispatch))==0
    assert lib.fvm_desktop_resource_frame_create(dispatch,b"/display/frame",16,C.byref(adapter),C.byref(target))==0
    assert lib.fvm_rdispatch_grant(dispatch,target,77,1)==0
    pixels=C.create_string_buffer(bytes(range(16)));source=b"memory://pixels"
    frame=Frame();frame.struct_size=C.sizeof(frame);frame.format=1;frame.reason=2;frame.width=frame.height=2
    frame.stride=8;frame.byte_count=16;frame.pixels=C.addressof(pixels);frame.source=source;frame.presentations=frame.total_presentations=1
    assert lib.fvm_desktop_resource_frame_capture(adapter,C.byref(frame))==0
    def call(identity,operation,data=b""):
        storage=C.create_string_buffer(data);q=Request(77,identity,target,operation,C.addressof(storage),len(data))
        assert lib.fvm_rdispatch_post(dispatch,C.byref(q))==0
        assert lib.fvm_rdispatch_pump(dispatch,1,C.byref(used))==0 and used.value==1
        assert lib.fvm_rdispatch_take(dispatch,77,identity,C.byref(reply))==0
        return reply.result,bytes(reply.data[:reply.size])
    assert call(1,1)==(0,payload(fields,source))
    acquired=list(fields);acquired[1:3]=[1,1]
    assert call(2,2)==(0,payload(acquired,source))
    pixels.raw=b"\xff"*17  # Native capture owns an independent pixel copy.
    assert call(3,3,b"".join(map(integer,[1,1,0,512])))==(0,bytes(range(16)))
    assert call(4,4,integer(1)+integer(1))==(0,b"")
    assert call(5,3,b"".join(map(integer,[1,1,0,16])))[0]==3
    assert lib.fvm_desktop_resource_frame_destroy(C.byref(adapter))==0
    assert lib.fvm_rdispatch_destroy(C.byref(dispatch))==0
    print("PASS independent frame metadata integers, strict geometry, atomic decoding and leased native pixels")


if __name__=="__main__":main()
