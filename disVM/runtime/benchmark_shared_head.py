"""Local full-profile cost for repeated list-head projection, no speedup claim."""
from benchmark_guarded_copies import main

GUEST = '''implement Cost;
include "sys.m";
Cost: module {init: fn();};
Node: adt {text: string; a: int; b: int;};
init() {
    sys := load Sys Sys->PATH;
    items := Node("reference", 3, 7) :: nil;
    result: Node;
    for (i := 0; i < 20000; i++) {
        result = hd items;
        result.a = i;
    }
    sys->print("PASS head cost %d %s %d\\n", result.a, result.text, result.b);
}
'''  # Same compiled native-cell source image for default/guarded startup.


if __name__ == '__main__':
    main(GUEST, "PASS head cost 19999 reference 7")
