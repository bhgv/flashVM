implement StringLength;
include "sys.m";

StringLength: module {
    init: fn();
};

# Build ASCII and Unicode strings and verify their logical character counts.
# No host pointers escape. Sys supplies output; the runtime owns ordinary string
# references, while the guarded LENC helper borrows cells only during one query.
init() {
    sys := load Sys Sys->PATH;
    ascii := "";
    unicode := "";
    for (i := 0; i < 32; i++) {
        # Rune storage uses a negative internal count. Both representations expose
        # the same nonnegative logical length to Limbo, independent of UTF-8 bytes.
        if (len ascii != i || len unicode != i) {
            raise "string length mismatch";
        }
        ascii += "a";
        unicode += "Ж";
    }
    # A normal uninitialized Limbo string is H: its length is zero. Internal C
    # NULL is a malformed string owner and raises exNilref instead.
    empty: string;
    if (len empty != 0) {
        raise "empty string mismatch";
    }
    sys->print("PASS string length32\n");
}
