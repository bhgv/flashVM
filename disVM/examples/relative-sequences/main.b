implement RelativeSequences;
include "sys.m";

RelativeSequences: module
{
    init: fn();
    Box: adt
    {
        number: int;
        text: string;
    };
};


# Exercise the native array/string operations under both actual VM dispatch modes.
# No arguments; local owners stay rooted across allocation and scheduler/GC polls.
# Failures raise; normal completion prints one PASS line through hosted Sys.
init()
{
    sys := load Sys Sys->PATH;
    words := array[] of {10, 20, 30};
    words[-1] = 42;
    words[-2:] = array[] of {20, 42};
    full := words[0:-1];
    full[0] = 99;
    if (len full != 3 || words[0] != 10 || full[-1] != 42) {
        raise "fail: independent int array copy";
    }
    tail := words[-2:-1];
    tail[-1] = 7;
    if (len tail != 2 || tail[0] != 20 || words[-1] != 42) {
        raise "fail: relative partial array copy";
    }
    bytes := array[] of {byte 1, byte 2, byte 3};
    bytes[-1] = byte 42;
    if (int bytes[-1] != 42) {
        raise "fail: relative byte index";
    }
    bigs := array[] of {big 1, big 42};
    reals := array[] of {1.0, 42.0};
    if (bigs[-1] != big 42 || reals[-1] != 42.0) {
        raise "fail: relative big/real index";
    }
    empty := array[0] of int;
    copied_empty := empty[0:-1];
    if (len copied_empty != 0) {
        raise "fail: empty array copy";
    }

    # Mapped record copies retain their strings after the source is retired.
    records := array[] of {Box(1, "Ж"), Box(2, "😀")};
    records_copy := records[0:-1];
    records = nil;
    if (records_copy[-1].text != "😀" || records_copy[-2].number != 1) {
        raise "fail: mapped record copy ownership";
    }
    shared := ref Box(7, "shared");
    refs := array[] of {shared, shared};
    refs_copy := refs[0:-1];
    refs_copy[-1].number = 8;
    if (refs[0].number != 8) {
        raise "fail: referenced objects retain identity";
    }

    text := "Ж42😀";
    original := text;
    if (text[-1] != 16r1f600 || text[-4] != 16r416) {
        raise "fail: Unicode scalar relative index";
    }
    text[-1] = 16r5a;
    if (original[-1] != 16r1f600 || text != "Ж42Z") {
        raise "fail: relative character write alias";
    }
    digits := text[-3:-2];
    all := text[0:-1];
    text[0] = 16r41;
    if (digits != "42" || all != "Ж42Z") {
        raise "fail: relative string slice copy";
    }
    nothing := "";
    if (len nothing[0:-1] != 0) {
        raise "fail: empty string copy";
    }
    sys->print("PASS relative arrays and Unicode strings: independent copies and rooted records\n");
}
