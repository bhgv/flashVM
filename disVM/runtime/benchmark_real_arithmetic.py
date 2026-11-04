"""Whole guarded-profile cost; arithmetic/guards/tails are not individually timed."""
from benchmark_guarded_copies import main

GUEST = '''implement RealArithmeticCost;
include "sys.m";
RealArithmeticCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[] of {2.0,4.0};
    score := 0.0;
    for (i := 0; i < 20000; i++) {
        a := values[i & 1];
        b := values[(i+1) & 1];
        score += a+b;
        score += a-b;
        score += a*b;
        score += a/b;
    }
    if (score != 305000.0)
    {
        raise "REAL arithmetic checksum";
    }
    sys->print("PASS REAL arithmetic cost\\n");
}
'''  # Eight arithmetic operations including accumulation, plus ordinary loop work.


if __name__ == '__main__':
    main(GUEST,'PASS REAL arithmetic cost')
