# Copyright (c) 2026 bhgv
# EN: Encode scalar and snapshot payloads without opening a transport.
# RU: Кодировать скаляры и снимки без открытия транспорта.
implement Styxvalue;
include "math.m";
include "styxvalue.m";

math: Math; # Immutable after initialization.
Wordbytes: con 8; # Payload width of a binary64 value.
Graphheader: con 16; # FVMG header byte count.
Graphversion: con 7; # Envelope version offset.
Graphsize: con 8; # Big-endian body length offset.
Graphcrc: con 12; # Big-endian checksum offset.
Crcpoly: con big 16redb88320; # Reflected IEEE CRC-32 polynomial.
Uintword: con big 16rffffffff; # Unsigned value mask of a binary word.


# EN/RU: See styxvalue.m. / См. styxvalue.m.
init()
{
    math = load Math Math->PATH;
    if (math == nil) {
        raise "cannot load Math for named values";
    }
}


# EN: Read one unsigned big-endian word as its exact int bit pattern.
# RU: Прочитать беззнаковое слово старшим байтом вперёд как точные биты int.
word(bytes: array of byte, at: int): int
{
    value := 0;
    for (i := 0; i < 4; ++i) {
        value = (value << 8) | int bytes[at+i];
    }
    return value;
}


# EN: Return the IEEE CRC-32 of borrowed bytes as an unsigned word.
# RU: Вернуть IEEE CRC-32 заимствованных байтов как беззнаковое слово.
crc(bytes: array of byte): big
{
    sum := Uintword;
    for (i := 0; i < len bytes; ++i) {
        sum ^= big bytes[i];
        for (bit := 0; bit < 8; ++bit) {
            sum = (sum >> 1) ^ (-(sum & big 1) & Crcpoly);
        }
    }
    return sum ^ Uintword;
}


# EN: Check the signature, version, exact length and checksum of a borrowed FVMG envelope.
# RU: Проверить сигнатуру, версию, точную длину и сумму заимствованной оболочки FVMG.
checkgraph(bytes: array of byte): int
{
    return len bytes > Graphheader && len bytes < Maxframe &&
        int bytes[0] == 'F' && int bytes[1] == 'V' && int bytes[2] == 'M' && int bytes[3] == 'G' &&
        int bytes[4] == 0 && int bytes[5] == 0 && int bytes[6] == 0 &&
        int bytes[Graphversion] >= 1 && int bytes[Graphversion] <= 9 &&
        word(bytes, Graphsize) == len bytes - Graphheader &&
        (big word(bytes, Graphcrc) & Uintword) == crc(bytes[Graphheader:]);
}


# EN: Copy one payload after a tag, enforcing the frame ceiling before allocation.
# RU: Скопировать тело после тега, проверяя предел кадра до выделения памяти.
enclose(tag: int, bytes: array of byte): array of byte
{
    if (len bytes >= Maxframe) {
        raise "named value is too long";
    }
    frame := array[len bytes+1] of byte;
    frame[0] = byte tag;
    if (len bytes) {
        frame[1:] = bytes;
    }
    return frame;
}


# EN: Return independent counted bytes from the frame's body.
# RU: Вернуть самостоятельные байты тела кадра с длиной.
payload(frame: array of byte): array of byte
{
    bytes := array[len frame-1] of byte;
    if (len bytes) {
        bytes[0:] = frame[1:];
    }
    return bytes;
}


# EN/RU: See styxvalue.m. / См. styxvalue.m.
text(value: string): ref Value
{
    return ref Value.Textual(array of byte value);
}


# EN/RU: See styxvalue.m. / См. styxvalue.m.
encode(value: ref Value): array of byte
{
    if (value == nil) {
        raise "missing named value";
    }
    pick item := value {
    Numeric =>
        bytes := array[Wordbytes+1] of byte;
        bytes[0] = byte Number;
        bits := math->realbits64(item.value);
        for (i := Wordbytes; i > 0; --i) {
            bytes[i] = byte bits;
            bits >>= 8;
        }
        return bytes;
    Textual =>
        return enclose(Text, item.bytes);
    Logical =>
        if (item.value != 0 && item.value != 1) {
            raise "invalid named Boolean";
        }
        return array[] of {byte Boolean, byte item.value};
    Nullval =>
        return array[] of {byte Null};
    Undefinedval =>
        return array[] of {byte Undefined};
    Graph =>
        if (!checkgraph(item.bytes)) {
            raise "invalid named graph envelope";
        }
        return enclose(Snapshot, item.bytes);
    }
    raise "unknown named value";
}


# EN/RU: See styxvalue.m. / См. styxvalue.m.
decode(frame: array of byte): ref Value
{
    if (len frame == 0 || len frame > Maxframe) {
        raise "invalid named frame size";
    }
    case int frame[0] {
    Number =>
        if (len frame != Wordbytes+1) {
            raise "invalid named binary64";
        }
        bits := big 0;
        for (i := 1; i < len frame; ++i) {
            bits = (bits << 8) | big frame[i];
        }
        return ref Value.Numeric(math->bits64real(bits));
    Text =>
        return ref Value.Textual(payload(frame));
    Boolean =>
        if (len frame == 2 && int frame[1] <= 1) {
            return ref Value.Logical(int frame[1]);
        }
    Null =>
        if (len frame == 1) {
            return ref Value.Nullval(0);
        }
    Undefined =>
        if (len frame == 1) {
            return ref Value.Undefinedval(0);
        }
    Snapshot =>
        if (checkgraph(frame[1:])) {
            return ref Value.Graph(payload(frame));
        }
    }
    raise "invalid named value frame";
}

include "styxvalue_graph_read.b";
include "styxvalue_graph_write.b";
