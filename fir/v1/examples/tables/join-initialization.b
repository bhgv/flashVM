implement JoinInitialization;

# Big selects the typed hybrid-table profile shared by CLI and C99 API clients.
JoinInitialization: module
{
    value: fn(): big; # Reference execution and both DisVM modes return 42.
};


# Declare typed locals before a condition, then initialize every continuing path.
# Reference storage starts as a mapped private nil cell, never as a fabricated
# table/string owner. FIR independently verifies a store before every load.
value(): big
{
    configuration := table of big {mode=1};
    answer: big;
    description: string;
    selected: table of big;
    count: int;

    if (configuration.mode == 1) {
        answer = big 40;
        description = "ready";
        selected = table of big {extra=2};
        count = int 1;
    } else {
        answer = big 21;
        description = "fallback";
        selected = table of big {extra=21};
        count = int 2;
    }

    # Both arms initialized all four locals, so reads at the join are valid.
    # Updating the selected table keeps its captured receiver under its lease.
    answer += selected.extra;
    selected.extra = big count;

    # The body-local reference cell is reset on each iteration. Both inner arms
    # store a valid string before the read; a previous iteration cannot supply it.
    for (index := int 0; index < int 2; index++) {
        local: string;
        if (index == int 0) {
            local = "first";
        } else {
            local = "second";
        }
        selected[big index] = big len local;
    }

    # Keep two explicit return sites for the observable result/flag harness.
    if (answer == 42 && description == "ready" && selected.extra == 1 &&
        selected[0] == 5 && selected[1] == 6) {
        return answer;
    } else {
        return 0;
    }
}
