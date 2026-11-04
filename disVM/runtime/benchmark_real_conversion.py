"""Whole-profile four-conversion workload; preserve all raw alternating samples."""
from benchmark_guarded_copies import main

GUEST = '''implement ConversionCost;
include "sys.m";
ConversionCost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    ints := array[] of {2,4};
    bigs := array[] of {big 3,big 5};
    reals := array[] of {2.5,3.5};
    score := 0.0;
    for (i := 0; i < 20000; i++) {
        index := i & 1;
        a := real ints[index];
        b := real bigs[index];
        c := int reals[index];
        d := big reals[index];
        if (c != index+3 || d != big (index+3))
        {
            raise "conversion integer checksum";
        }
        score += a+b;
    }
    if (score != 140000.0)
    {
        raise "conversion REAL checksum";
    }
    sys->print("PASS FP conversion cost\\n");
}
'''  # Four conversions, two REAL additions plus index/ordinary loop/check work.


if __name__ == '__main__':
    main(GUEST,'PASS FP conversion cost',retain_samples=True)
