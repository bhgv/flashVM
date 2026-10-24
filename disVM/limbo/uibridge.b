implement Uibridge;
include "uibridge.m";


# Encode the bits of value into exactly eight lowercase ASCII digits.
# value is a Limbo int; returns a new string, including two's-complement negatives.
word(value: int): string
{
    digits := "0123456789abcdef";
    result := "";
    for (i := 0; i < 8; i++) {
        result = string digits[value & 15] + result;
        value >>= 4;
    }
    return result;
}


# Parse one fixed-width word from an already bounded frame.
# frame borrows bytes; offset names its eight-digit field. Returns (ok, int bits).
readword(frame: array of byte, offset: int): (int, int)
{
    value := 0;
    for (i := 0; i < 8; i++) {
        digit := int frame[offset + i];
        if (digit >= '0' && digit <= '9') {
            digit -= '0';
        } else if (digit >= 'a' && digit <= 'f') {
            digit = digit - 'a' + 10;
        } else {
            return (0, 0);
        }
        value = (value << 4) | digit;
    }
    return (1, value);
}


# Check common envelope class and correlation, independently of service permissions.
# message is borrowed and may be nil. Returns one for valid common fields, otherwise zero.
valid(message: ref Message): int
{
    if (message == nil || message.kind < Request || message.kind > Focus) {
        return 0;
    }
    for (i := 0; i < len message.text; i++) {
        scalar := message.text[i];
        if (scalar < 0 || scalar > 16r10ffff || (scalar >= 16rd800 && scalar <= 16rdfff)) {
            return 0;
        }
    }
    if (message.kind == Request || message.kind == Reply) {
        return message.id != 0;
    }
    return message.id == 0;
}


# Serialize a common record into a fresh byte array; never share native structs.
# message is borrowed; returns (1, owned frame) or (0, nil), as declared in uibridge.m.
encode(message: ref Message): (int, array of byte)
{
    if (!valid(message)) {
        return (0, nil);
    }
    payload := array of byte message.text;
    if (len payload > MaxText) {
        return (0, nil);
    }
    header := "fvd1:" + word(message.kind) + word(message.id) + word(message.code)
        + word(message.flags) + word(message.first) + word(message.second);
    frame := array[Header + len payload] of byte;
    frame[0:] = array of byte header;
    frame[Header:] = payload;
    return (1, frame);
}


# Decode a borrowed transport frame without modifying it or performing I/O.
# Returns (1, owned record) or (0, nil); callers retain rejected frames for diagnostics.
decode(frame: array of byte): (int, ref Message)
{
    if (len frame < Header || len frame > Header + MaxText) {
        return (0, nil);
    }
    if (string frame[0:5] != "fvd1:") {
        return (0, nil);
    }
    fields := array[6] of int;
    for (i := 0; i < 6; i++) {
        (ok, value) := readword(frame, 5 + i * 8);
        if (!ok) {
            return (0, nil);
        }
        fields[i] = value;
    }
    message := ref Message(fields[0], fields[1], fields[2], fields[3], fields[4],
        fields[5], string frame[Header:]);
    if (!valid(message)) {
        return (0, nil);
    }
    # Round-trip validation prevents malformed UTF-8 from being silently replaced.
    payload := array of byte message.text;
    if (len payload != len frame - Header) {
        return (0, nil);
    }
    for (i = 0; i < len payload; i++) {
        if (payload[i] != frame[Header + i]) {
            return (0, nil);
        }
    }
    return (1, message);
}
