"""Whole-profile LENL cost with two prebuilt chains and no loop allocation."""
from benchmark_guarded_copies import main

GUEST = '''implement ListCost;
include "sys.m";
ListCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    chains := array[2] of list of int;
    for (i := 0; i < 32; i++) {
        chains[1] = i :: chains[1];
        if (i < 31) {
            chains[0] = i :: chains[0];
        }
    }
    total := 0;
    for (j := 0; j < 20000; j++) {
        current := chains[j & 1];
        total += len current;
    }
    if (total != 630000) {
        raise "list length checksum";
    }
    sys->print("PASS list length cost\\n");
}
'''  # Dynamic31/32-element chains; constructors are outside the measured loop.


if __name__ == '__main__':
    main(GUEST, 'PASS list length cost')
