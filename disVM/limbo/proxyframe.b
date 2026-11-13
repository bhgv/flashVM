# Copyright (c) 2026 bhgv
# EN: Canonical fixed-size network codec, bounded before every field access.
# RU: Canonical fixed-size network codec с проверками до доступа к полям.
implement Proxyframe;
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxyframe.m";


# EN/RU: Refuse malformed record/offset before accessing any bytes.
# Отказать при неверных record/offset до доступа к bytes.
check(bytes: array of byte, offset: int)
{
    if (len bytes != Size || offset < 8 || offset > Size - 8 || offset % 8 != 0) {
        raise "invalid proxy frame field";
    }
}


# EN/RU: Read unsigned bit pattern at offset; signed big preserves all wire bits.
# Прочитать bit pattern по offset; signed big сохраняет все wire bits.
get(bytes: array of byte, offset: int): big
{
    check(bytes, offset);
    value := big 0;
    for (index := 0; index < 8; index++) {
        value = (value << 8) | big int bytes[offset + index];
    }
    return value;
}


# EN/RU: Encode value bit pattern, independent of host endianness and WORD width.
# Кодировать value независимо от host endianness и WORD width.
put(bytes: array of byte, offset: int, value: big)
{
    check(bytes, offset);
    for (index := 7; index >= 0; index--) {
        bytes[offset + index] = byte (int value & 255);
        value >>= 8;
    }
}


# EN/RU: Allocate canonical header with explicit byte count and connection epoch.
# Выделить canonical header с явными byte count и connection epoch.
make(kind: int, high, low: big): array of byte
{
    bytes := array[Size] of byte;
    for (index := 0; index < Size; index++) {
        bytes[index] = byte 0;
    }
    bytes[0] = byte 'D';
    bytes[1] = byte 'V';
    bytes[2] = byte 'P';
    bytes[3] = byte '1';
    bytes[4] = byte 1;
    bytes[5] = byte kind;
    put(bytes, 8, big Size);
    put(bytes, EpochHi, high);
    put(bytes, EpochLo, low);
    return bytes;
}


# EN/RU: Return a nonnegative bounded-slot handle; zero identity is reserved for absent refs.
# Вернуть handle с ограниченным slot; нулевая identity зарезервирована для отсутствующих refs.
handle(bytes: array of byte, offset: int): Proxyowner->Handle
{
    session := get(bytes, offset);
    slot := get(bytes, offset + 8);
    generation := get(bytes, offset + 16);
    if (session < big 0 || generation < big 0 || slot < big 0 || slot > big 255) {
        raise "invalid proxy wire handle";
    }
    return Proxyowner->Handle(session, int slot, generation);
}


# EN/RU: Serialize caller-owned handle values, never a native address.
# Сериализовать значения handle caller, не native address.
sethandle(bytes: array of byte, offset: int, handle: Proxyowner->Handle)
{
    put(bytes, offset, handle.session);
    put(bytes, offset + 8, big handle.slot);
    put(bytes, offset + 16, handle.generation);
}


# EN/RU: Validate fixed framing and trusted-connection incarnation before parsing payload.
# Проверить fixed framing и incarnation доверенного соединения до разбора payload.
valid(bytes: array of byte, kind: int, high, low: big): int
{
    return len bytes == Size && bytes[0] == byte 'D' && bytes[1] == byte 'V' &&
        bytes[2] == byte 'P' && bytes[3] == byte '1' && bytes[4] == byte 1 &&
        bytes[5] == byte kind && bytes[6] == byte 0 && bytes[7] == byte 0 &&
        get(bytes, 8) == big Size && get(bytes, EpochHi) == high && get(bytes, EpochLo) == low;
}


# EN/RU: Exact request equality, or request/reply correlation excluding result bytes.
# Точное равенство request либо request/reply correlation без result bytes.
same(left, right: array of byte, response: int): int
{
    if (len left != Size || len right != Size) {
        return 0;
    }
    limit := Size;
    if (response) {
        limit = Status;
    }
    for (index := 0; index < limit; index++) {
        if ((!response || index != 5) && left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}


# EN/RU: Copy immutable request into an independently owned reply and scalar outcome.
# Копировать immutable request в отдельный reply с scalar outcome.
answer(request: array of byte, status, value: int): array of byte
{
    bytes := array[Size] of byte;
    bytes[0:] = request;
    bytes[5] = byte Reply;
    put(bytes, Status, big status);
    put(bytes, Result, big value);
    return bytes;
}
