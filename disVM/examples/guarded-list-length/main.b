implement ListLength;
include "sys.m";
ListLength: module {
    init: fn(); # Entry: no arguments or returned ownership.
};

# Own the two chains through ordinary local references. Sys printing may block;
# failed checks raise guest exceptions after native guard cleanup. No spawn is
# used: this example checks results, not general shared-heap parallel admission.
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN list length\n");
    # Empty typed lists initially use the Dis H sentinel.
    values: list of int;
    names: list of string;
    for (i := 0; i < 32; i++) {
        # Constructors transfer tails into new heads; frame owners keep them live.
        values = i :: values;
        names = "kept" :: names; # Pointer-bearing payloads remain GC-managed.
        if (len values != i+1 || len names != i+1) {
            raise "list length mismatch";
        }
    }
    # An empty H list returns zero. Internal NULL is a malformed-list error.
    empty: list of int;
    if (len empty != 0) {
        raise "empty list length";
    }
    sys->print("PASS list length32\n");
}
