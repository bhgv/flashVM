"""Whole-profile LENC cost with ASCII/Rune owners preallocated before the loop."""
from benchmark_guarded_copies import main

GUEST = '''implement StringCost;
include "sys.m";
StringCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[4] of string;
    for (i := 0; i < 32; i++) {
        values[1] += "a";
        values[3] += "Ж";
        if (i < 31) {
            values[0] += "a";
            values[2] += "Ж";
        }
    }
    total := 0;
    for (j := 0; j < 20000; j++) {
        current := values[j & 3];
        total += len current;
    }
    if (total != 630000) {
        raise "string length checksum";
    }
    sys->print("PASS string length cost\\n");
}
'''  # Two representations, length31/32; no string builders in the read loop.


if __name__ == '__main__':
    main(GUEST, 'PASS string length cost')
