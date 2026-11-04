"""Whole-profile cost of six comparisons over preallocated REAL values."""
from benchmark_guarded_copies import main

GUEST = '''implement RealBranchCost;
include "sys.m";
RealBranchCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[] of {0.0, -1.0, 1.0, 2.5};
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
        raise "real branch checksum";
    }
    sys->print("PASS real branch cost\\n");
}
'''  # Dynamic array selection; six conditions and ordinary loop work, no builders.


if __name__ == '__main__':
    main(GUEST, 'PASS real branch cost')
