#!/usr/bin/env python3
"""Fragmented/coalesced transport oracle: bounded buffering and verified-view lifetimes."""
import ctypes as C
import sys
from srecord_oracle import Record,frame,integer
class Stream(C.Structure):
    _fields_=[('struct_size',C.c_size_t),('buffer',C.c_void_p),('capacity',C.c_size_t),('limit',C.c_size_t),
        ('filled',C.c_size_t),('expected',C.c_size_t),('ready',C.c_int),('error',C.c_int)]

def main():
    lib=C.CDLL(sys.argv[1]);init,push,reset=lib.fvm_sstream_init,lib.fvm_sstream_push,lib.fvm_sstream_reset
    init.argtypes=[C.POINTER(Stream),C.c_void_p,C.c_size_t,C.c_size_t]
    push.argtypes=[C.POINTER(Stream),C.c_void_p,C.c_size_t,C.POINTER(Record),C.POINTER(C.c_size_t)]
    reset.argtypes=[C.POINTER(Stream)]
    stream=Stream();storage=C.create_string_buffer(20000);used=C.c_size_t();rec=Record()
    for payload in [b'',b'\0\xff'*50,bytes(range(256))*33,frame(4,b'nested')]:
        data=frame(8192,payload)
        for chunk in [1,2,3,7,64,4096,len(data)+5]:
            assert init(C.byref(stream),storage,len(storage),16000)==0
            position=0
            while position<len(data):
                part=data[position:position+chunk];rec=Record(19,20,21,22)
                status=push(C.byref(stream),part,len(part),C.byref(rec),C.byref(used));position+=used.value
                assert used.value==len(part)
                if position<len(data):
                    assert status==1 and (rec.tag,rec.payload,rec.size,rec.encoded_size)==(19,20,21,22)
                else:
                    assert status==0 and rec.tag==8192 and C.string_at(rec.payload,rec.size)==payload
            before=storage.raw;used.value=777
            assert push(C.byref(stream),b'x',1,C.byref(rec),C.byref(used))==2 and used.value==0
            assert storage.raw==before
            assert reset(C.byref(stream))==0 and not stream.filled and not stream.ready
    one,two=frame(3,b'first'),frame(4,b'second');combined=one+two
    assert push(C.byref(stream),combined,len(combined),C.byref(rec),C.byref(used))==0 and used.value==len(one)
    assert reset(C.byref(stream))==0
    assert push(C.byref(stream),two,len(two),C.byref(rec),C.byref(used))==0 and rec.tag==4
    for bad,status in [(b'\xc0',2),(integer(0)+integer(0),2),(integer(0)+integer(20000),3),
        (one[:-1]+bytes([one[-1]^1]),4)]:
        assert reset(C.byref(stream))==0;rec=Record(19,20,21,22)
        assert push(C.byref(stream),bad,len(bad),C.byref(rec),C.byref(used))==status
        assert rec.tag==19 and rec.payload==20
        saved=storage.raw;used.value=777
        assert push(C.byref(stream),one,len(one),C.byref(rec),C.byref(used))==status
        assert used.value==0 and storage.raw==saved
    small=C.create_string_buffer(b'\xa5'*10)
    assert init(C.byref(stream),small,3,16000)==0
    assert push(C.byref(stream),frame(3,b'x'*100),103,C.byref(rec),C.byref(used))==3
    assert used.value<=3 and small.raw[3:10]==b'\xa5'*7
    assert init(C.byref(stream),storage,len(storage),16000)==0
    rec=Record(19,20,21,22);used.value=777
    assert push(C.byref(stream),None,0,C.byref(rec),C.byref(used))==1 and used.value==0 and rec.tag==19
    assert init(C.byref(stream),small,3,1)==0
    minimal=frame(0,b'');assert len(minimal)==3
    assert push(C.byref(stream),minimal,3,C.byref(rec),C.byref(used))==0
    assert rec.tag==0 and rec.size==0 and used.value==3
    print('PASS arbitrary fragments, coalesced frames, sticky errors, header quotas, stable views and explicit reset')

if __name__=='__main__':main()
