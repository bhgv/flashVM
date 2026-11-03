implement Branches;
include "sys.m";
Branches: module {
    init: fn(); # Entry point; no arguments or returned ownership.
};

# Own all arrays/scalars until entry returns; no shared references or channels.
# Check six predicates for each of three native widths. Sys printing may block;
# failures raise an ordinary guest exception, outside native object guards.
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN branches\n");
    # Each ordered pair satisfies exactly three of its six comparisons.
    score := 0;
    for (i := 0; i < 8; i++) {
        # Arrays force typed runtime operands rather than folded constants.
        b := array[2] of byte;
        w := array[2] of int;
        l := array[2] of big;
        # Byte order is unsigned; the other two families use signed order.
        b[0] = byte (i & 31);
        b[1] = byte ((i & 31)+1);
        w[0] = -(i & 31)-1;
        w[1] = i & 31;
        l[0] = big -9000000007-big (i & 31);
        l[1] = big 9000000007;
        # All six BYTE predicates, including both equality alternatives.
        if (b[0] == b[1]) {
            score++;
        }
        if (b[0] != b[1]) {
            score++;
        }
        if (b[0] < b[1]) {
            score++;
        }
        if (b[0] <= b[1]) {
            score++;
        }
        if (b[0] > b[1]) {
            score++;
        }
        if (b[0] >= b[1]) {
            score++;
        }
        # WORD predicates cross zero and retain negative signed ordering.
        if (w[0] == w[1]) {
            score++;
        }
        if (w[0] != w[1]) {
            score++;
        }
        if (w[0] < w[1]) {
            score++;
        }
        if (w[0] <= w[1]) {
            score++;
        }
        if (w[0] > w[1]) {
            score++;
        }
        if (w[0] >= w[1]) {
            score++;
        }
        # LONG operands exceed 32 bits, with the same signed predicate rules.
        if (l[0] == l[1]) {
            score++;
        }
        if (l[0] != l[1]) {
            score++;
        }
        if (l[0] < l[1]) {
            score++;
        }
        if (l[0] <= l[1]) {
            score++;
        }
        if (l[0] > l[1]) {
            score++;
        }
        if (l[0] >= l[1]) {
            score++;
        }
    }
    # Eight rounds, three widths, three true predicates:8*3*3 =72.
    if (score != 72) {
        raise "branch mismatch";
    }
    sys->print("PASS branches18\n");
}
