"""Whole-profile cost of six comparisons over preallocated ASCII/Rune strings."""
from benchmark_guarded_copies import main

GUEST = '''implement StringBranchCost;
include "sys.m";
StringBranchCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[] of {"a", "ab", "Ж", "Жx"};
    score := 0;
    for (i := 0; i < 20000; i++) {
        a := values[i & 3];
        b := values[(i+1) & 3];
        if (a == b)
        {
            score++;
        }
        if (a != b)
        {
            score++;
        }
        if (a < b)
        {
            score++;
        }
        if (a <= b)
        {
            score++;
        }
        if (a > b)
        {
            score++;
        }
        if (a >= b)
        {
            score++;
        }
    }
    if (score != 60000)
    {
        raise "string branch checksum";
    }
    sys->print("PASS string branch cost\\n");
}
'''  # Dynamic array selection; six conditions and ordinary loop work, no builders.


if __name__ == '__main__':
    main(GUEST, 'PASS string branch cost')
