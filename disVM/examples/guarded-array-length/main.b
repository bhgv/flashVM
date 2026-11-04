implement ArrayLength;
include "sys.m";
ArrayLength: module {
    init: fn(); # Entry point: no arguments or returned ownership.
};

# Own each typed array until its local reference is replaced. Sys output can
# block; failed checks raise normal guest exceptions after native guards close.
# No spawn is used here, so this is not a general shared-heap parallelism demo.
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN array length\n");
    # Exercise genuine arrays, including allocated arrays of length zero.
    for (i := 0; i < 32; i++) {
        # Element shape differs; the array header length remains native WORD.
        b := array[i] of byte;
        w := array[i] of int;
        r := array[i] of string; # Mapped reference elements initialize to nil.
        if (len b != i || len w != i || len r != i) {
            raise "array length mismatch";
        }
    }
    # A nil reference is distinct from an allocated empty array, but len is zero.
    n: array of byte;
    if (len n != 0) {
        raise "nil length mismatch";
    }
    sys->print("PASS array length32\n");
}
