implement DecimalappendTest;
include "sys.m";
include "math.m";

DecimalappendTest: module
{
    init: fn(); # Exercise refreshed decimal/string semantics on the actual embedded Dis host.
};


# Convert a runtime string operand rather than permitting a compile-time numeric fold.
# source is borrowed; returns the actual Dis real conversion, including signed zero.
parse(source: string): real
{
    return real source;
}


# Check real conversions, rounding controls and shared/Unicode append ownership.
# No arguments; restores the original rounding control before successful exit.
init()
{
    sys := load Sys Sys->PATH;
    math := load Math Math->PATH;
    if (sys == nil || math == nil) {
        raise "fail: decimal/append modules";
    }
    inputs := array[] of {"0", "-0", "+0", "123456789", "-999999999", "+42", "", "1.25", "1e2", " 12", "123456789012345", "-999999999999999"};
    expected := array[] of {0.0, 0.0, 0.0, 123456789.0, -999999999.0, 42.0, 0.0, 1.25, 100.0, 12.0, 123456789012345.0, -999999999999999.0};
    original := math->getFPcontrol();
    for (rounding := 0; rounding < 4; rounding++) {
        math->FPcontrol(rounding << 8, Math->RND_MASK);
        for (index := 0; index < len inputs; index++) {
            result := parse(inputs[index]);
            if (result != expected[index]) {
                raise "fail: decimal real value";
            }
            if (result == 0.0 && (math->realbits64(result) < big 0) != (index == 1)) {
                raise "fail: decimal signed zero";
            }
            if ((math->getFPcontrol() & Math->RND_MASK) != (rounding << 8)) {
                raise "fail: decimal changed rounding control";
            }
        }
    }
    math->FPcontrol(original, Math->RND_MASK);
    integers := array[] of {"999999999999999999", "-999999999999999999", "+123456789012345678"};
    expectedbig := array[] of {big 999999999999999999, big -999999999999999999, big 123456789012345678};
    for (number := 0; number < len integers; number++) {
        if (big integers[number] != expectedbig[number]) {
            raise "fail: wide decimal big conversion";
        }
    }
    joined := "";
    for (piece := 0; piece < 64; piece++) {
        joined += "ab";
        if (len joined != (piece + 1) * 2) {
            raise "fail: ASCII concatenation length";
        }
    }
    for (position := 0; position < len joined; position++) {
        if (joined[position] != 'a' + position % 2) {
            raise "fail: ASCII concatenation contents";
        }
    }
    retained := joined;
    joined += joined;
    if (len retained != 128 || len joined != 256 || joined[:128] != retained || joined[128:] != retained) {
        raise "fail: shared/self ASCII concatenation";
    }
    text := "";
    for (index := 0; index < 64; index++) {
        previous := text; # A retained owner forces copy-on-write where required.
        character := 'a' + index % 26;
        text[len text] = character; # Real INSC append, not host-side string concatenation.
        if (len previous != index || len text != index + 1 || text[index] != character) {
            raise "fail: append length/retained owner";
        }
        for (offset := 0; offset < len previous; offset++) {
            if (text[offset] != previous[offset]) {
                raise "fail: append changed prior characters";
            }
        }
    }
    ascii := text; # Preserve the ASCII representation across later promotion.
    text[len text] = 16r20ac;
    text[len text] = 16r1f600;
    if (len ascii != 64 || len text != 66 || text[64] != 16r20ac || text[65] != 16r1f600) {
        raise "fail: Unicode append/promotion";
    }
    if (!(ascii < text) || text == ascii || text[:64] != ascii || text != text) {
        raise "fail: refreshed string comparison/slice";
    }
    sys->print("PASS actual Dis wide real/big decimals, signed zero, rounding, ASCII concatenate and shared/Unicode append\n");
}
