implement StringSliceTest;
include "sys.m";

StringSliceTest: module
{
    init: fn(); # Verify imported slice semantics on the actual NC64 runtime, without a UI host.
};


# Return the independently predictable scalar at source position index.
# wide selects mixed Rune/non-BMP content; zero selects ASCII. No ownership or I/O is involved.
character(index, wide: int): int
{
    if (wide) {
        if (index % 3 == 0) {
            return 16r1f600 + index % 32;
        }
        return 16r410 + index % 32;
    }
    return '!' + index % 80;
}


# Allocate and return one local source value of count scalars, choosing its representation with wide.
# The caller receives the only application reference; the builder never publishes an alias.
make(count, wide: int): string
{
    value := "";
    for (index := 0; index < count; index++) {
        value[len value] = character(index, wide);
    }
    return value;
}


# Check the complete result against scalar formulas, not a second use of the slicing operation.
# value is borrowed; first/count describe the source interval. A mismatch raises visibly.
check(value: string, first, count, wide: int)
{
    if (len value != count) {
        raise "fail: NC64 slice scalar extent";
    }
    for (index := 0; index < count; index++) {
        if (value[index] != character(first + index, wide)) {
            raise "fail: NC64 slice scalar contents";
        }
    }
}


# Exercise prefix/interior, unique/shared, empty and native-boundary results on real compiled Dis.
# No parameters; each local root is released before this test process returns. No native API is called.
init()
{
    sys := load Sys Sys->PATH;
    if (sys == nil) {
        raise "fail: NC64 slice Sys";
    }
    counts := array[] of {0, 1, 127, 128, 129, 256};
    for (wide := 0; wide < 2; wide++) {
        for (offset := 0; offset < 2; offset++) {
            first := offset * 7;
            for (index := 0; index < len counts; index++) {
                count := counts[index];
                value := make(first + count + 9, wide);
                # Same-slot slicing with no application alias admits the unique-owner path.
                value = value[first:first + count];
                check(value, first, count, wide);
                # A live second root must retain the entire original source after slicing the first.
                value = make(first + count + 9, wide);
                alias := value;
                value = value[first:first + count];
                check(value, first, count, wide);
                check(alias, 0, first + count + 9, wide);
                # [GC:] Both local references remain alive through their content checks only.
            }
        }
    }
    sys->print("PASS NC64 string slices: ASCII/Rune, prefix/interior, unique/shared, 0/1/127/128/129/256\n");
}
