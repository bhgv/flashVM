implement Idevalue;
include "idevalue.m";


# Allocate a canonical scalar with no retained mutable input; kind/payload are checked by copy.
# integer/number/text supply their respective typed payload. Returns an owned descriptor.
scalar(kind: int, integer: big, number: real, text: string): ref Value
{
    return ref Value(kind, integer, number, text, "", 0, "", nil, nil);
}


# Copy a borrowed non-table entry; returns nil for invalid kind, payload or identity.
# Object/Method are identity copies only: no receiver state, getter or function executes here.
entry(value: ref Value): ref Value
{
    if (value == nil) {
        return nil;
    }
    case value.kind {
    Undefined or Null =>
        return scalar(value.kind, big 0, 0.0, "");
    Bool =>
        if (value.integer == big 0 || value.integer == big 1) {
            return scalar(Bool, value.integer, 0.0, "");
        }
    Int =>
        if (intbits() == 64 || value.integer >= -big 2147483647 - big 1 && value.integer <= big 2147483647) {
            return scalar(Int, value.integer, 0.0, "");
        }
    Byte =>
        if (value.integer >= big 0 && value.integer <= big 255) {
            return scalar(Byte, value.integer, 0.0, "");
        }
    Big =>
        return scalar(Big, value.integer, 0.0, "");
    Real =>
        return scalar(Real, big 0, value.number, "");
    Char =>
        if (value.integer >= big 0 && value.integer <= big 16r10ffff
           && !(value.integer >= big 16rd800 && value.integer <= big 16rdfff)) {
            return scalar(Char, value.integer, 0.0, "");
        }
    Text =>
        if (len value.text <= MaxText) {
            return scalar(Text, big 0, 0.0, value.text);
        }
    Object or Method =>
        if (value.epoch > 0 && len value.owner > 0 && len value.owner <= 256
           && len value.member <= 256 && (value.kind == Method && len value.member > 0
                                         || value.kind == Object && value.member == "")) {
            return ref Value(value.kind, big 0, 0.0, "", value.owner, value.epoch,
                             value.member, nil, nil);
        }
    }
    return nil;
}


# Copy a validated shallow table, or one scalar/reference. value is borrowed until return.
# [GC:] New arrays and entry records detach caller mutation; reference descriptors retain identity only.
# Returns nil on duplicate names, quotas, invalid entries or any nested/cyclic table.
copy(value: ref Value): ref Value
{
    if (value == nil) {
        return nil;
    }
    if (value.kind != Table) {
        return entry(value);
    }
    count := len value.names;
    if (count > MaxFields || len value.values != count) {
        return nil;
    }
    names := array[count] of string;
    values := array[count] of ref Value;
    total := 0;
    for (i := 0; i < count; i++) {
        name := value.names[i];
        if (len name == 0 || len name > MaxText) {
            return nil;
        }
        for (j := 0; j < i; j++) {
            if (names[j] == name) {
                return nil;
            }
        }
        item := entry(value.values[i]);
        if (item == nil) {
            return nil;
        }
        total += len name + len item.text + len item.owner + len item.member;
        if (total > MaxTableText) {
            return nil;
        }
        names[i] = name;
        values[i] = item;
    }
    return ref Value(Table, big 0, 0.0, "", "", 0, "", names, values);
}


# Parse an exact signed decimal big without using real/AS Number or overflowing an intermediate.
# text is borrowed; optional sign and a bounded digit sequence are admitted, without whitespace/trailing data.
# Returns success and the signed value; failure returns (0, 0), including either overflow extreme.
decimal(text: string): (int, big)
{
    if (len text == 0 || len text > 20) {
        return (0, big 0);
    }
    negative := 0;
    first := 0;
    if (text[0] == '-' || text[0] == '+') {
        negative = text[0] == '-';
        first = 1;
    }
    if (first == len text) {
        return (0, big 0);
    }
    limit := -big 9223372036854775807;
    if (negative) {
        limit -= big 1;
    }
    value := big 0;
    for (i := first; i < len text; i++) {
        digit := text[i] - '0';
        if (digit < 0 || digit > 9 || value < limit / big 10) {
            return (0, big 0);
        }
        value *= big 10;
        if (value < limit + big digit) {
            return (0, big 0);
        }
        value -= big digit;
    }
    if (!negative) {
        value = -value;
    }
    return (1, value);
}


# Convert one validated scalar to a checked integer. value is borrowed; no object is evaluated.
# Finite integral reals within the exact binary64 integer range are admitted; others fail.
# Returns success and big payload, preserving exact text/Big inputs beyond 2^53.
integer(value: ref Value): (int, big)
{
    case value.kind {
    Bool or Int or Big or Char or Byte =>
        return (1, value.integer);
    Text =>
        return decimal(value.text);
    Real =>
        number := value.number;
        if (number != number || number < -9007199254740991.0 || number > 9007199254740991.0) {
            return (0, big 0);
        }
        result := big number;
        if (real result == number) {
            return (1, result);
        }
    }
    return (0, big 0);
}


# Convert a borrowed canonical scalar, returning an independent value or nil on rejection.
# Strict conversions are library operations; they do not replace native AS coercion semantics.
convert(value: ref Value, target: int): ref Value
{
    source := copy(value);
    if (source == nil) {
        return nil;
    }
    if (source.kind == target) {
        return source;
    }
    if (source.kind == Object || source.kind == Method || source.kind == Table) {
        return nil;
    }
    if (target == Int || target == Big || target == Char || target == Byte) {
        if (target == Char && source.kind == Text && len source.text == 1) {
            return entry(scalar(Char, big source.text[0], 0.0, ""));
        }
        (ok, result) := integer(source);
        if (!ok) {
            return nil;
        }
        return entry(scalar(target, result, 0.0, ""));
    }
    if (target == Real) {
        case source.kind {
        Bool or Int or Big or Char or Byte =>
            # Precision loss here is explicit; Big itself was never transported through real.
            return scalar(Real, big 0, real source.integer, "");
        Text =>
            (ok, result) := floating(source.text);
            if (ok) {
                return scalar(Real, big 0, result, "");
            }
        }
        return nil;
    }
    if (target == Bool) {
        truth := 0;
        case source.kind {
        Bool or Int or Big or Char or Byte =>
            truth = source.integer != big 0;
        Real =>
            truth = source.number == source.number && source.number != 0.0;
        Text =>
            truth = len source.text != 0;
        }
        return scalar(Bool, big truth, 0.0, "");
    }
    if (target == Text) {
        text := "";
        case source.kind {
        Undefined =>
            text = "undefined";
        Null =>
            text = "null";
        Bool =>
            text = "false";
            if (source.integer != big 0) {
                text = "true";
            }
        Int or Big or Byte =>
            text = string source.integer;
        Real =>
            text = string source.number;
        Char =>
            text = " ";
            text[0] = int source.integer;
        }
        return scalar(Text, big 0, 0.0, text);
    }
    return nil;
}


# Validate a bounded complete decimal float before invoking the VM's binary64 parser.
# text is borrowed; optional sign, decimal point and exponent are allowed, but no whitespace/hex/junk.
# Returns (0, 0.0) for malformed/nonfinite text; finite underflow to signed zero is allowed.
floating(text: string): (int, real)
{
    if (len text == 0 || len text > 128) {
        return (0, 0.0);
    }
    index := 0;
    if (text[index] == '+' || text[index] == '-') {
        index++;
    }
    digits := 0;
    while (index < len text && text[index] >= '0' && text[index] <= '9') {
        index++;
        digits++;
    }
    if (index < len text && text[index] == '.') {
        index++;
        while (index < len text && text[index] >= '0' && text[index] <= '9') {
            index++;
            digits++;
        }
    }
    if (digits == 0) {
        return (0, 0.0);
    }
    if (index < len text && (text[index] == 'e' || text[index] == 'E')) {
        index++;
        if (index < len text && (text[index] == '+' || text[index] == '-')) {
            index++;
        }
        first := index;
        while (index < len text && text[index] >= '0' && text[index] <= '9') {
            index++;
        }
        if (index == first) {
            return (0, 0.0);
        }
    }
    if (index != len text) {
        return (0, 0.0);
    }
    result := real text;
    if (result != result || result > 1.7976931348623157e308 || result < -1.7976931348623157e308) {
        return (0, 0.0);
    }
    return (1, result);
}


# Detect the compiled Limbo integer profile using a defined big-to-int narrowing conversion.
# Returns 32 for classic Dis, 64 for NC64; independent of the native host's pointer width.
intbits(): int
{
    if (int (big 1 << 32) != 0) {
        return 64;
    }
    return 32;
}


# Reflect canonical targets and the actual compiled int profile into fresh metadata records.
# No arguments. Returns owned descriptors; changing them cannot alter subsequent reflection.
types(): array of ref Type
{
    width := intbits();
    return array[] of {
        ref Type(Byte, "byte", "byte", "tobyte", 8, 1),
        ref Type(Int, "int", "int", "toint", width, 1),
        ref Type(Big, "big", "big", "tobig", 64, 1),
        ref Type(Real, "real", "real", "toreal", 64, 1),
        ref Type(Text, "string", "string", "tostring", 0, 1),
        ref Type(Bool, "bool", "int", "tobool", width, 1),
        ref Type(Char, "char", "int", "tochar", width, 1),
        ref Type(Table, "table", "named snapshot", "totable", 0, 1),
        ref Type(Object, "object", "owner reference", "", 0, 0),
        ref Type(Method, "method", "bound owner reference", "", 0, 0)
    };
}
