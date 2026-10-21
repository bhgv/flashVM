#!/usr/bin/env python3
"""Independent resource request/reply wire vectors and authenticated-session/queue boundaries."""
import ctypes as C
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'protocol'))
from srecord_oracle import frame,integer
from sbindings_oracle import Table,Binding,declaration
class Handle(C.Structure):_fields_=[('id',C.c_ulong),('generation',C.c_ulong)]
class Operation(C.Structure):_fields_=[('id',C.c_ulong),('right',C.c_uint)]
class Request(C.Structure):_fields_=[('session',C.c_ulong),('id',C.c_ulong),('target',Handle),('operation',C.c_ulong),('data',C.c_void_p),('size',C.c_size_t)]
class Reply(C.Structure):_fields_=[('result',C.c_int),('size',C.c_size_t),('data',C.c_ubyte*512)]
Handler=C.CFUNCTYPE(C.c_int,C.POINTER(Request),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t),C.c_void_p)

def main():
    lib=C.CDLL(sys.argv[1]);d=C.c_void_p();handle=Handle();used=C.c_size_t();calls=[]
    lib.fvm_rdispatch_create.argtypes=[C.c_void_p,C.POINTER(C.c_void_p)]
    lib.fvm_rdispatch_register.argtypes=[C.c_void_p,C.c_char_p,C.POINTER(Operation),C.c_size_t,Handler,C.c_void_p,C.POINTER(Handle)]
    lib.fvm_rdispatch_grant.argtypes=[C.c_void_p,Handle,C.c_ulong,C.c_uint]
    lib.fvm_rdispatch_pump.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    lib.fvm_rdispatch_take.argtypes=[C.c_void_p,C.c_ulong,C.c_ulong,C.POINTER(Reply)]
    lib.fvm_rdispatch_unregister.argtypes=[C.c_void_p,Handle]
    lib.fvm_rdispatch_destroy.argtypes=[C.POINTER(C.c_void_p)]
    lib.fvm_sbindings_init.argtypes=[C.POINTER(Table),C.POINTER(Binding),C.c_size_t]
    lib.fvm_sbindings_apply.argtypes=[C.POINTER(Table),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    write,post,rwrite,read=lib.fvm_rrequest_record_write,lib.fvm_rdispatch_post_record,lib.fvm_rreply_record_write,lib.fvm_rreply_record_read
    write.argtypes=[C.c_ulong,C.POINTER(Request),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    post.argtypes=[C.c_void_p,C.c_ulong,C.POINTER(Table),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    rwrite.argtypes=[C.c_ulong,C.c_ulong,C.POINTER(Reply),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    read.argtypes=[C.POINTER(Table),C.c_void_p,C.c_size_t,C.POINTER(C.c_ulong),C.POINTER(Reply),C.POINTER(C.c_size_t)]
    @Handler
    def echo(q,out,capacity,written,_):
        calls.append((q.contents.session,q.contents.id));data=C.string_at(q.contents.data,q.contents.size)
        C.memmove(out,data,len(data));written[0]=len(data);return 0
    assert lib.fvm_rdispatch_create(None,C.byref(d))==0
    ops=(Operation*2)(Operation(1,1),Operation(2,2))
    assert lib.fvm_rdispatch_register(d,b'/scene/root/value',ops,2,echo,None,C.byref(handle))==0
    assert lib.fvm_rdispatch_grant(d,handle,77,1)==0
    tables=[];storage=[]
    for name in [b'flashvm.resource.request',b'flashvm.resource.reply',b'boolean']:
        table=Table();slots=(Binding*1)();storage.append(slots)
        assert lib.fvm_sbindings_init(C.byref(table),slots,1)==0
        metadata=declaration([(3,name,1)])
        assert lib.fvm_sbindings_apply(C.byref(table),metadata,len(metadata),C.byref(used))==0
        tables.append(table)
    payload=C.create_string_buffer(b'\0hello\xff');q=Request(999,1,handle,1,C.addressof(payload),7)
    out=C.create_string_buffer(1024)
    assert write(3,C.byref(q),out,len(out),C.byref(used))==0
    packet=out.raw[:used.value]
    body=b''.join(map(integer,[1,1,handle.id,handle.generation,1,7]))+b'\0hello\xff'
    assert packet==frame(3,body)
    for cut in range(len(packet)):
        used.value=777
        assert post(d,77,C.byref(tables[0]),packet,cut,C.byref(used))==6 and used.value==777
    assert post(d,77,C.byref(tables[0]),packet,len(packet),C.byref(used))==0 and used.value==len(packet) and not calls
    used.value=777
    assert post(d,77,C.byref(tables[0]),packet,len(packet),C.byref(used))==5 and used.value==777
    assert lib.fvm_rdispatch_pump(d,1,C.byref(used))==0 and used.value==1 and calls==[(77,1)]
    reply=Reply();assert lib.fvm_rdispatch_take(d,78,1,C.byref(reply))==3
    assert lib.fvm_rdispatch_take(d,77,1,C.byref(reply))==0 and reply.size==7 and bytes(reply.data[:7])==b'\0hello\xff'
    assert rwrite(3,1,C.byref(reply),out,len(out),C.byref(used))==0
    response=out.raw[:used.value];assert response==frame(3,b''.join(map(integer,[1,1,0,7]))+b'\0hello\xff')
    rid=C.c_ulong(999);decoded=Reply();assert read(C.byref(tables[1]),response,len(response),C.byref(rid),C.byref(decoded),C.byref(used))==0 and rid.value==1
    invalid_reply=Reply();invalid_reply.result=6;invalid_reply.size=0;used.value=777
    assert rwrite(3,1,C.byref(invalid_reply),out,len(out),C.byref(used))==2 and used.value==777
    pending_packet=frame(3,b''.join(map(integer,[1,1,6,0])))
    assert read(C.byref(tables[1]),pending_packet,len(pending_packet),C.byref(rid),C.byref(decoded),C.byref(used))==2
    before=bytes(decoded);rid.value=999;used.value=777
    assert read(C.byref(tables[0]),response,len(response),C.byref(rid),C.byref(decoded),C.byref(used))==2 and rid.value==999 and bytes(decoded)==before and used.value==777
    for table,status in [(tables[0],4),(tables[1],1),(tables[2],1)]:
        # A wire session claim never exists: authenticated argument 78 cannot use session 77's grant.
        used.value=777
        assert post(d,78,C.byref(table),packet,len(packet),C.byref(used))==status and used.value==777
    bad=packet[:-1]+bytes([packet[-1]^1]);assert post(d,77,C.byref(tables[0]),bad,len(bad),C.byref(used))==1
    for badbody in [integer(2)+body[1:],body+b'x',b''.join(map(integer,[1,2,handle.id,handle.generation,2,0]))]:
        badpacket=frame(3,badbody);expected=4 if badbody.endswith(integer(2)+integer(0)) else 1
        assert post(d,77,C.byref(tables[0]),badpacket,len(badpacket),C.byref(used))==expected
    assert len(calls)==1
    assert lib.fvm_rdispatch_unregister(d,handle)==0
    assert post(d,77,C.byref(tables[0]),packet,len(packet),C.byref(used))==3
    assert lib.fvm_rdispatch_destroy(C.byref(d))==0 and not d.value
    print('PASS independent request/reply bytes, directional schemas, authenticated sessions, CRC/prefix rejection and no pre-pump effects')

if __name__=='__main__':main()
