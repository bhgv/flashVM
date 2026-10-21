#!/usr/bin/env python3
"""Independent integer/zlib feedback bytes and Python-integer adaptation reference model."""
import ctypes as C
import random
import sys
from srecord_oracle import frame, integer
from scontrol_oracle import Limits


class Feedback(C.Structure):
    _fields_=[(n,C.c_ulong) for n in ('stream','epoch','window','expected','on_time','late','damaged','rtt_known','rtt_ms')]


class Config(C.Structure):
    _fields_=[('struct_size',C.c_size_t),('profiles',C.POINTER(Limits)),('count',C.c_size_t),('initial',C.c_size_t)]+[
        (n,C.c_ulong) for n in ('stream','epoch','bad_percent','good_percent','bad_rtt_ms','good_rtt_ms',
                               'bad_windows','good_windows','cooldown_windows','min_expected')]


class State(C.Structure):
    _fields_=[('struct_size',C.c_size_t),('config',Config),('profiles',Limits*8),('current',C.c_size_t),
              ('pending',C.c_size_t)]+[(n,C.c_ulong) for n in ('last_window','bad_run','good_run','cooldown')]


class Decision(C.Structure):
    _fields_=[('action',C.c_uint),('reasons',C.c_ulong),('current',C.c_size_t),('candidate',C.c_size_t),('limits',Limits)]


def main():
    lib=C.CDLL(sys.argv[1]);ci,init,observe=lib.fvm_sadapt_config_init,lib.fvm_sadapt_init,lib.fvm_sadapt_observe
    commit,reject=lib.fvm_sadapt_commit,lib.fvm_sadapt_reject
    write,read=lib.fvm_sadapt_feedback_write,lib.fvm_sadapt_feedback_read
    ci.argtypes=[C.POINTER(Config)];init.argtypes=[C.POINTER(State),C.POINTER(Config)]
    observe.argtypes=[C.POINTER(State),C.POINTER(Feedback),C.POINTER(Decision)]
    commit.argtypes=[C.POINTER(State),C.POINTER(Limits)];reject.argtypes=[C.POINTER(State)]
    write.argtypes=[C.POINTER(Feedback),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    read.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Feedback),C.POINTER(C.c_size_t)]
    profiles=(Limits*3)(Limits(19,480,512,400),Limits(19,992,1024,768),Limits(19,4064,4096,4000))
    config=Config();assert ci(C.byref(config))==0
    config.profiles=profiles;config.count=3;config.initial=1;config.stream=7;config.epoch=8
    state=State();assert init(C.byref(state),C.byref(config))==0
    assert C.addressof(state.config.profiles.contents)==C.addressof(state.profiles)
    output=C.create_string_buffer(128);used=C.c_size_t(9999);decoded=Feedback()
    feedback=Feedback(7,8,1,0xffffffff,0xffffffff-1,1,0,1,20)
    values=[3,1]+[getattr(feedback,n) for n,_ in Feedback._fields_]
    expected=frame(0,b''.join(map(integer,values)))
    assert write(C.byref(feedback),output,128,C.byref(used))==0 and output.raw[:used.value]==expected
    assert read(expected+expected,len(expected)*2,C.byref(decoded),C.byref(used))==0
    assert used.value==len(expected) and bytes(decoded)==bytes(feedback)
    before,prior=bytes(decoded),used.value
    for cut in range(len(expected)):
        assert read(expected,cut,C.byref(decoded),C.byref(used))==1
        assert bytes(decoded)==before and used.value==prior
    for fields,status in [([1,0],6),([2,1],6),([3,99],6),([3,1],2),
                          ([3,1,7,8,0,10,10,0,0,1,20],2),([3,1,7,8,1,10,9,1,1,1,20],2),
                          ([3,1,7,8,1,10,10,0,0,0,20],2),([3,1,7,8,1,10,10,0,0,1,20,0],2)]:
        raw=frame(0,b''.join(map(integer,fields)))
        assert read(raw,len(raw),C.byref(decoded),C.byref(used))==status
        assert bytes(decoded)==before and used.value==prior
    # Reference model uses arbitrary-precision multiplication, independent from native threshold division.
    rng=random.Random(745); current=1;pending=8;bad_run=good_run=cooldown=0;decision=Decision()
    for window in range(1,1201):
        count=rng.choice([1,63,64,65,100,101,0xffffffff])
        bad_count=rng.choice([0,1,(count*10)//100,(count*10+99)//100,count//2,count])
        bad_count=min(count,bad_count)
        known=rng.randrange(2);rtt=rng.choice([0,20,50,51,249,250,500]) if known else 0
        f=Feedback(7,8,window,count,count-bad_count,0,0,known,rtt)
        candidate=current;action=reason=0
        if count<64:action=3;bad_run=good_run=0
        elif cooldown:cooldown-=1;bad_run=good_run=0;reason=8
        else:
            reason=(1 if bad_count*100>=count*10 else 0)|(2 if known and rtt>=250 else 0)
            bad=bool(reason);good=not bad and known and rtt<=50 and bad_count*100<=count
            bad_run=bad_run+1 if bad else 0;good_run=good_run+1 if good else 0
            if bad_run>=2:
                bad_run=good_run=0
                if current:action=1;candidate=current-1
            elif good_run>=4:
                bad_run=good_run=0;reason=4
                if current<2:action=2;candidate=current+1
        assert observe(C.byref(state),C.byref(f),C.byref(decision))==0
        assert (decision.action,decision.reasons,decision.current,decision.candidate)==(action,reason,current,candidate),window
        assert (state.bad_run,state.good_run,state.cooldown)==(bad_run,good_run,cooldown)
        saved,old=bytes(state),bytes(decision)
        assert observe(C.byref(state),C.byref(f),C.byref(decision))==5
        assert bytes(state)==saved and bytes(decision)==old
        if action in (1,2):
            assert state.current==current and state.pending==candidate
            if rng.randrange(4)==0:assert reject(C.byref(state))==0
            else:
                wrong=Limits(20,profiles[candidate].record_limit,profiles[candidate].slot_bytes,profiles[candidate].part_bytes)
                assert commit(C.byref(state),C.byref(wrong))==5 and bytes(state)==saved
                assert commit(C.byref(state),C.byref(profiles[candidate]))==0;current=candidate
            pending=8;cooldown=2;bad_run=good_run=0
        assert state.current==current and state.pending==pending
    saved,old=bytes(state),bytes(decision)
    for f,status in [(Feedback(7,9,1201,100,100,0,0,1,20),5),
                     (Feedback(7,8,1201,100,99,1,1,1,20),2),
                     (Feedback(7,8,1201,0,0,0,0,0,0),2)]:
        assert observe(C.byref(state),C.byref(f),C.byref(decision))==status
        assert bytes(state)==saved and bytes(decision)==old
    print('PASS independent compact feedback bytes and 1200 overflow-safe adaptation windows')


if __name__=='__main__':main()
