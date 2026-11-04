implement Reals;
include "sys.m";
Reals: module {
    init: fn();
};
# Compare dynamic REAL values using all six existing branch conditions.
# Inputs are byte-snapshotted together, then evaluated outside shared guards.
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN real branches\n");
    # This example uses finite values. Special IEEE values/status are covered
    # separately by the actual-runtime diagnostic fixture.
    values := array[] of {0.0, -1.0, 1.0, 2.5};
    # Exactly three predicates hold for any ordered finite pair.
    score := 0;
    for (i := 0; i < 4; i++) {
        for (j := 0; j < 4; j++) {
            if (values[i] == values[j])
            {
                score++;
            }
            if (values[i] != values[j])
            {
                score++;
            }
            if (values[i] < values[j])
            {
                score++;
            }
            if (values[i] <= values[j])
            {
                score++;
            }
            if (values[i] > values[j])
            {
                score++;
            }
            if (values[i] >= values[j])
            {
                score++;
            }
        }
    }
    if (score != 48)
    {
        raise "real branch score";
    }
    sys->print("PASS real branches6\n");
}
