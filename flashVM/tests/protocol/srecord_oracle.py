#!/usr/bin/env python3
"""Independent Python big-integer/zlib oracle for the native C89 S-record codec."""
import ctypes as C
import random
import struct
import sys
import zlib

class Integer(C.Structure):
    _fields_ = [('high', C.c_ulong), ('low', C.c_ulong), ('negative', C.c_int)]
class Record(C.Structure):
    _fields_ = [('tag', C.c_ulong), ('payload', C.c_void_p), ('size', C.c_size_t), ('encoded_size', C.c_size_t)]

def integer(n):
    negative=n<0
    n=abs(n)
    groups=[]
    while n>63:
        groups.insert(0,n%128)
        n//=128
    groups.insert(0,n | (64 if negative else 0))
    groups[-1]|=128
    return bytes(groups)

def frame(tag,payload):
    body=integer(tag)+payload
    length=integer(len(body))
    mask=(1<<min(32,6+7*(len(length)-1)))-1
    return integer(zlib.crc32(length+body)&mask)+length+body

def main():
    lib=C.CDLL(sys.argv[1])
    iw,ir=lib.fvm_sinteger_write,lib.fvm_sinteger_read
    iw.argtypes=[C.POINTER(Integer),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    ir.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Integer),C.POINTER(C.c_size_t)]
    rw,rr=lib.fvm_srecord_write,lib.fvm_srecord_read
    rw.argtypes=[C.c_ulong,C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    rr.argtypes=[C.c_void_p,C.c_size_t,C.c_size_t,C.POINTER(Record),C.POINTER(C.c_size_t)]
    for n,expected in [(0,b'\x80'),(63,b'\xbf'),(-63,b'\xff'),(64,b'\x00\xc0'),(-64,b'\x40\xc0'),(8191,b'\x3f\xff'),(8192,b'\x00\x40\x80')]:
        assert integer(n)==expected
    rng=random.Random(722)
    values=[0,1,63,64,127,128,8191,8192,2**32-1,2**32,2**63,2**64-1]
    values += [rng.getrandbits(64) for _ in range(2000)]
    for magnitude in values:
        for sign in ([1,-1] if magnitude else [1]):
            n=magnitude*sign;value=Integer(magnitude>>32,magnitude&0xffffffff,n<0)
            out=C.create_string_buffer(10);used=C.c_size_t(999)
            assert iw(C.byref(value),out,10,C.byref(used))==0
            encoded=out.raw[:used.value];assert encoded==integer(n),(n,encoded)
            decoded=Integer();consumed=C.c_size_t()
            assert ir(encoded,len(encoded),C.byref(decoded),C.byref(consumed))==0
            assert (decoded.high,decoded.low,decoded.negative,consumed.value)==(value.high,value.low,value.negative,len(encoded))
            for cut in range(len(encoded)):
                decoded=Integer(11,22,1);consumed.value=777
                assert ir(encoded,cut,C.byref(decoded),C.byref(consumed))==1
                assert (decoded.high,decoded.low,decoded.negative,consumed.value)==(11,22,1,777)
    for bad in [b'\xc0',b'\x00\x81',b'\x40\x81',b'\x00'*10,b'\x3f'+b'\x7f'*8+b'\xff']:
        decoded=Integer(11,22,1);used=C.c_size_t(999)
        assert ir(bad,len(bad),C.byref(decoded),C.byref(used))==2,bad
        assert used.value==999 and decoded.low==22
    payloads=[b'',b'\0\xff',struct.pack('>d',-1.25),frame(3,b'\x81')]
    payloads += [bytes(rng.randrange(256) for _ in range(n)) for n in [1,62,63,64,127,128,8190,8191,8192,65536]]
    for tag in [0,3,63,64,8191,8192,2**32-1]:
        for payload in payloads:
            expected=frame(tag,payload);out=C.create_string_buffer(len(expected));used=C.c_size_t()
            assert rw(tag,payload,len(payload),out,len(out),C.byref(used))==0
            assert out.raw==expected,(tag,len(payload))
            rec=Record();consumed=C.c_size_t()
            assert rr(out,len(out),16777216,C.byref(rec),C.byref(consumed))==0
            assert (rec.tag,rec.size,consumed.value)==(tag,len(payload),len(expected))
            assert C.string_at(rec.payload,rec.size)==payload
            # Every header split and selected large-payload split must retain untouched outputs.
            for cut in sorted(set(range(min(18,len(expected))))|{len(expected)-1}):
                rec=Record(19,20,21,22);consumed.value=777
                assert rr(out,cut,16777216,C.byref(rec),C.byref(consumed))==1
                assert (rec.tag,rec.payload,rec.size,rec.encoded_size,consumed.value)==(19,20,21,22,777)
            untouched=C.create_string_buffer(b'\xa5'*len(expected));used.value=777
            assert rw(tag,payload,len(payload),untouched,len(expected)-1,C.byref(used))==3
            assert untouched.raw[:len(expected)]==b'\xa5'*len(expected) and used.value==777
    # Exercise the three-to-four-byte length/20-to-27-bit checksum boundary explicitly.
    for size in [1048574,1048575]:
        payload=b'\xa5'*size;expected=frame(1,payload);out=C.create_string_buffer(len(expected));used=C.c_size_t()
        assert rw(1,payload,size,out,len(out),C.byref(used))==0 and out.raw==expected
        rec=Record();assert rr(out,len(out),16777216,C.byref(rec),C.byref(used))==0
    # Oversized geometry must be rejected before dereferencing the advertised payload extent.
    out=C.create_string_buffer(b'\xa5'*32);used=C.c_size_t(777)
    assert rw(3,b'x',C.c_size_t(-1).value,out,32,C.byref(used))==3
    assert out.raw[:32]==b'\xa5'*32 and used.value==777
    for length,limit,status in [(16777216,16777216,1),(16777217,16777216,3),(16777216,16777215,3)]:
        prefix=integer(0)+integer(length);rec=Record(19,20,21,22);used.value=777
        assert rr(prefix,len(prefix),limit,C.byref(rec),C.byref(used))==status and rec.tag==19 and used.value==777
    good=frame(3,b'abc');bad=good[:-1]+bytes([good[-1]^1]);rec=Record(19,20,21,22);used=C.c_size_t(777)
    assert rr(bad,len(bad),100,C.byref(rec),C.byref(used))==4
    assert rec.tag==19 and used.value==777
    assert rr(good,len(good),1,C.byref(rec),C.byref(used))==3
    for malformed_body in [b'\xc1',b'\x00\x81',b'\x00']:
        length=integer(len(malformed_body));data=integer(zlib.crc32(length+malformed_body)&63)+length+malformed_body
        assert rr(data,len(data),100,C.byref(rec),C.byref(used))==2
    # Concatenation consumes exactly one verified record, leaving the next for the caller.
    combined=good+frame(9,b'next')
    assert rr(combined,len(combined),100,C.byref(rec),C.byref(used))==0 and used.value==len(good)
    print('PASS 64-bit sign/magnitude, canonical prefixes, independent CRC wire vectors, nested/binary payloads and atomic rejection')

if __name__=='__main__':main()
