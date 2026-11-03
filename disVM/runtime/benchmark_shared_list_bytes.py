"""Local whole-profile cost of five scalar/raw head projections."""
from benchmark_guarded_copies import main

GUEST = '''implement Cost;
include "sys.m";
Cost: module {init: fn();};
Plain: adt {a: int; b: int;};
init() {
    sys := load Sys Sys->PATH;
    bs := byte 137 :: nil;
    ws := 731 :: nil;
    ls := big 9000000007 :: nil;
    fs := 3.5 :: nil;
    ps := Plain(3, 7) :: nil;
    b: byte;
    w: int;
    l: big;
    f: real;
    p: Plain;
    for (i := 0; i < 20000; i++) {
        b = hd bs;
        w = hd ws;
        l = hd ls;
        f = hd fs;
        p = hd ps;
    }
    if (int b != 137 || w != 731 || l != big 9000000007 || f != 3.5 || p.a != 3 || p.b != 7) {
        raise "scalar list benchmark checksum";
    }
    sys->print("PASS scalar-list cost 137 731 3 7\\n");
}
'''  # Same native-cell image and checksums for both startup selections.


if __name__ == '__main__':
    main(GUEST, "PASS scalar-list cost 137 731 3 7")
