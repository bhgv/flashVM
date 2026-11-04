"""Whole-profile cost of discovered array-length reads, with allocations outside loop."""
from benchmark_guarded_copies import main

GUEST = '''implement LengthCost;
include "sys.m";
LengthCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[2] of array of int;
    values[0] = array[31] of int;
    values[1] = array[32] of int;
    total := 0;
    for (i := 0; i < 20000; i++) {
        current := values[i & 1];
        total += len current;
    }
    if (total != 630000) {
        raise "array length checksum";
    }
    sys->print("PASS array length cost\\n");
}
'''  # Dynamic source alternates31/32; no array allocation inside the measured loop.


if __name__ == '__main__':
    main(GUEST, 'PASS array length cost')
