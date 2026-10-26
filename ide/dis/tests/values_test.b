# Regression for explicit values derived from the Limbo type system.
implement Valuescheck;
include "sys.m";
include "idevalue.m";


Valuescheck: module
{
    init: fn(); # Native Dis test entry.
};
v: Idevalue; # Loaded pure typed conversion module, shared read-only by this test.


# Construct one scalar descriptor; kind/payload are borrowed values, no native object escapes.
value(kind: int, integer: big, number: real, text: string): ref Idevalue->Value
{
    return ref Idevalue->Value(kind, integer, number, text, "", 0, "", nil, nil);
}


# Check a complete decimal integer conversion at exact signed 64-bit boundaries.
# text is input; expected is the signed result. Raises a test failure on rejected/rounded values.
checkbig(text: string, expected: big)
{
    result := v->convert(value(Idevalue->Text, big 0, 0.0, text), Idevalue->Big);
    if (result == nil || result.integer != expected) {
        raise "fail: exact signed big conversion";
    }
    rendered := v->convert(result, Idevalue->Text);
    if (rendered == nil || rendered.text != text) {
        raise "fail: exact big rendering";
    }
}


# Require conversion rejection. input remains caller-owned; no partial result is admitted.
reject(input: ref Idevalue->Value, target: int)
{
    if (v->convert(input, target) != nil) {
        raise "fail: invalid conversion accepted";
    }
}


# Exercise exact Big, profile-dependent Int, binary64, Unicode and detached table ownership.
# All checks execute in the real interpreter/JIT; no host-language conversion oracle is substituted.
init()
{
    s := load Sys Sys->PATH;
    v = load Idevalue Idevalue->PATH;
    if (s == nil || v == nil) {
        raise "fail: value modules unavailable";
    }
    reflected := v->types();
    if (len reflected != 10 || reflected[1].bits != v->intbits() || reflected[3].name != "real"
       || reflected[3].bits != 64 || reflected[8].copied || reflected[9].copied) {
        raise "fail: Limbo type reflection";
    }
    reflected[1].bits = 1;
    fresh := v->types();
    if (fresh[1].bits != v->intbits()) {
        raise "fail: mutable reflected catalog alias";
    }
    maximum := big 9223372036854775807;
    minimum := -maximum - big 1;
    checkbig("9223372036854775807", maximum);
    checkbig("-9223372036854775808", minimum);
    checkbig("9007199254740993", big 9007199254740993);
    reject(value(Idevalue->Text, big 0, 0.0, "9223372036854775808"), Idevalue->Big);
    reject(value(Idevalue->Text, big 0, 0.0, "-9223372036854775809"), Idevalue->Big);
    reject(value(Idevalue->Text, big 0, 0.0, "12x"), Idevalue->Big);
    reject(value(Idevalue->Text, big 0, 0.0, " 12"), Idevalue->Big);
    wide := v->convert(value(Idevalue->Big, maximum, 0.0, ""), Idevalue->Int);
    if (v->intbits() == 64) {
        if (wide == nil || wide.integer != maximum) {
            raise "fail: NC64 int profile narrowed";
        }
    } else if (v->intbits() != 32 || wide != nil) {
        raise "fail: classic int profile overflow";
    }
    reject(value(Idevalue->Real, big 0, 1.25, ""), Idevalue->Int);
    reject(value(Idevalue->Real, big 0, 9007199254740992.0, ""), Idevalue->Big);
    realvalue := v->convert(value(Idevalue->Text, big 0, 0.0, "-1.25e2"), Idevalue->Real);
    if (realvalue == nil || realvalue.number != -125.0) {
        raise "fail: tofloat/toreal decimal conversion";
    }
    rounded := v->convert(value(Idevalue->Big, big 9007199254740993, 0.0, ""), Idevalue->Real);
    if (rounded == nil || rounded.number != 9007199254740992.0) {
        raise "fail: explicit binary64 rounding";
    }
    for (bad := 0; bad < 5; bad++) {
        inputs := array[] of {"1e309", "1e", "1.2tail", "NaN", "0x1p2"};
        reject(value(Idevalue->Text, big 0, 0.0, inputs[bad]), Idevalue->Real);
    }
    bytevalue := v->convert(value(Idevalue->Big, big 255, 0.0, ""), Idevalue->Byte);
    if (bytevalue == nil || bytevalue.integer != big 255) {
        raise "fail: byte boundary";
    }
    reject(value(Idevalue->Big, big 256, 0.0, ""), Idevalue->Byte);
    reject(value(Idevalue->Big, -big 1, 0.0, ""), Idevalue->Byte);
    character := v->convert(value(Idevalue->Text, big 0, 0.0, "😀"), Idevalue->Char);
    if (character == nil || character.integer != big 16r1f600) {
        raise "fail: Unicode scalar conversion";
    }
    text := v->convert(character, Idevalue->Text);
    if (text == nil || text.text != "😀") {
        raise "fail: Unicode scalar rendering";
    }
    reject(value(Idevalue->Big, big 16rd800, 0.0, ""), Idevalue->Char);
    reject(value(Idevalue->Big, big 16r110000, 0.0, ""), Idevalue->Char);
    truth := v->convert(value(Idevalue->Text, big 0, 0.0, "false"), Idevalue->Bool);
    if (truth == nil || truth.integer != big 1) {
        raise "fail: nonempty string truthiness";
    }
    falsehood := v->convert(value(Idevalue->Null, big 0, 0.0, ""), Idevalue->Bool);
    if (falsehood == nil || falsehood.integer != big 0) {
        raise "fail: null truthiness";
    }
    fields := array[] of {value(Idevalue->Big, maximum, 0.0, "")};
    names := array[] of {"large"};
    table := ref Idevalue->Value(Idevalue->Table, big 0, 0.0, "", "", 0, "", names, fields);
    copied := v->copy(table);
    if (copied == nil) {
        raise "fail: explicit table copy";
    }
    fields[0].integer = big 0;
    names[0] = "changed";
    if (copied.names[0] != "large" || copied.values[0].integer != maximum) {
        raise "fail: mutable value alias";
    }
    # Reject an intentionally cyclic table without following it recursively.
    fields[0] = table;
    if (v->copy(table) != nil) {
        raise "fail: cyclic table admitted";
    }
    fields[0] = nil; # Break the test-only cycle after verifying rejection.
    proxy := ref Idevalue->Value(Idevalue->Object, big 0, 0.0, "", "target", 3, "", nil, nil);
    reference := v->copy(proxy);
    if (reference == nil || reference.owner != "target" || reference.epoch != 3) {
        raise "fail: reference identity";
    }
    reject(proxy, Idevalue->Text);
    s->print("PASS Limbo values: int%d big real byte string Unicode snapshots references\n", v->intbits());
}
