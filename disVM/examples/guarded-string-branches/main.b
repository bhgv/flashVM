implement Strings;
include "sys.m";
Strings: module {
    init: fn();
};
# Compare logical characters, not UTF-8 encodings. Runtime owns the strings;
# each query borrows protected owner/header/prefix spans until its PC is chosen.
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN string branches\n");
    # ASCII and Unicode representations exercise all six relational operators.
    values := array[] of {"", "a", "Ж", "ab"};
    # Exactly three of the six predicates hold for any ordered pair.
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
    # Reject wrong aggregate results before reporting success.
    if (score != 48)
    {
        raise "string branch score";
    }
    sys->print("PASS string branches6\n");
}
