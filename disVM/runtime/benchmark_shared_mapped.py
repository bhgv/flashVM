"""Measure local opt-in mapped replacement cost, never parallel speedup."""
from benchmark_guarded_copies import main

GUEST = '''implement Cost;
include "sys.m";
Cost: module {init: fn();};
Node: adt {text: string; a: int; b: int;};
init() {
    sys := load Sys Sys->PATH;
    records := array[2] of Node;
    records[0] = Node("reference", 3, 7);
    for (i := 0; i < 20000; i++) {
        records[0].a = i;
        records[1] = records[0];
    }
    sys->print("PASS mapped cost %d %s %d\\n", records[1].a, records[1].text, records[1].b);
}
'''  # Same native-cell image for both startup profiles.


if __name__ == '__main__':
    main(GUEST, "PASS mapped cost 19999 reference 7")
