"""Local whole-profile cost of HEADP/TAIL; not isolated opcode timing."""
from benchmark_guarded_copies import main

GUEST = '''implement Cost;
include "sys.m";
Cost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    items := "front" :: "back" :: nil;
    first: string;
    last: string;
    rest: list of string;
    for (i := 0; i < 20000; i++) {
        first = hd items;
        rest = tl items;
        last = hd rest;
    }
    sys->print("PASS pointer-list cost %s %s\\n", first, last);
}
'''  # Identical native-cell image for ordinary/guarded startup.


if __name__ == '__main__':
    main(GUEST, "PASS pointer-list cost front back")
