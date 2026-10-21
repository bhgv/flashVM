#!/usr/bin/env python3
"""Independent per-part model for native receive observations and canonical feedback output."""
import ctypes as C
import random
import sys
from sadapt_oracle import Feedback
from srecord_oracle import frame, integer

class Measure(C.Structure):
    _fields_ = [('struct_size',C.c_size_t), ('report',Feedback),
                ('deadline_ms',C.c_ulong), ('last_ms',C.c_ulong), ('closed',C.c_int),
                ('categories',C.c_ubyte*1024)]


def main():
    lib = C.CDLL(sys.argv[1])
    init, record, finish = lib.fvm_smeasure_init, lib.fvm_smeasure_record, lib.fvm_smeasure_finish
    init.argtypes = [C.POINTER(Measure), *([C.c_ulong]*5)]
    record.argtypes = [C.POINTER(Measure),C.c_ulong,C.c_uint,C.c_ulong]
    finish.argtypes = [C.POINTER(Measure),*([C.c_ulong]*3),C.POINTER(Feedback)]
    write = lib.fvm_sadapt_feedback_write
    write.argtypes = [C.POINTER(Feedback),C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
    rng = random.Random(747)
    arrivals = 0
    for window in range(1,81):
        expected = rng.choice([1,2,3,4,5,63,64,65,4095,4096])
        deadline = rng.choice([1,20,100,1000])
        s = Measure(); assert init(C.byref(s),7,1,window,expected,deadline) == 0
        # The model uses sets/categories, not native packed two-bit operations.
        statuses = ['missing']*expected
        elapsed = 0
        for _ in range(600):
            elapsed += rng.randrange(3)
            part,valid = rng.randrange(expected),rng.randrange(2)
            category = 'damaged' if not valid else 'on_time' if elapsed <= deadline else 'late'
            previous = statuses[part]
            if previous != 'on_time' and (category == 'on_time' or previous in ('missing','damaged')):
                statuses[part] = category
            assert record(C.byref(s),part,valid,elapsed) == 0
            assert (s.report.on_time,s.report.late,s.report.damaged) == tuple(statuses.count(v) for v in ('on_time','late','damaged'))
            before = bytes(s)
            assert record(C.byref(s),expected,valid,elapsed) == 2 and bytes(s) == before
            if elapsed:
                assert record(C.byref(s),part,valid,elapsed-1) == 5 and bytes(s) == before
            arrivals += 1
        report = Feedback(); known = window%2; rtt = 20 if known else 0
        assert finish(C.byref(s),max(elapsed,deadline),known,rtt,C.byref(report)) == 0
        values = [3,1,7,1,window,expected,*[statuses.count(v) for v in ('on_time','late','damaged')],known,rtt]
        wire = frame(0,b''.join(map(integer,values)))
        output = C.create_string_buffer(len(wire));used = C.c_size_t(999)
        assert write(C.byref(report),output,len(output),C.byref(used)) == 0 and output.raw == wire
        before = bytes(s); untouched = bytes(report)
        assert finish(C.byref(s),max(elapsed,deadline),known,rtt,C.byref(report)) == 5
        assert record(C.byref(s),0,1,elapsed) == 5
        assert bytes(s) == before and bytes(report) == untouched
    print('PASS independent unique-part model:',arrivals,'arrivals and 80 checked feedback records')


if __name__ == '__main__':main()
