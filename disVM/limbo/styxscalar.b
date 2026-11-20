# Copyright (c) 2026 bhgv
# EN: Convert one synchronous scalar call into ordinary VFS reads and writes.
# RU: Преобразовать синхронный скалярный вызов в чтение и запись через VFS.
implement Styxscalar;
include "sys.m";
include "styxscalar.m";


# EN: Write a positive 32-bit S-integer ending at end, returning its start.
# RU: Записать положительное 32-битное S-число перед end и вернуть его начало.
field(bytes: array of byte, end: int, number: big): int
{
    end--;
    bytes[end] = byte (number & big 127) | byte 128;
    while (number >= big 128) {
        number >>= 7;
        end--;
        bytes[end] = byte (number & big 127);
    }
    # The leading group has six magnitude bits.
    if ((bytes[end] & byte 64) != byte 0) {
        end--;
        bytes[end] = byte 0;
    }
    return end;
}


# EN: Compute the S-record short-body CRC over length, tag and payload.
# RU: Вычислить CRC короткой S-записи по длине, тегу и данным.
checksum(bytes: array of byte): byte
{
    crc := big 16rFFFFFFFF;
    for (i := 0; i < len bytes; i++) {
        crc ^= big bytes[i];
        for (bit := 0; bit < 8; bit++) {
            if ((crc & big 1) != big 0) {
                crc = (crc >> 1) ^ big 16rEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return byte ((~crc & big 63) | big 128);
}


# EN: Encode one canonical scalar S-record with an eight-byte signed payload.
# RU: Закодировать каноническую S-запись с восьмибайтовым знаковым значением.
frame(tag: big, kind: byte, value: big): array of byte
{
    bytes := array[16] of byte;
    at := field(bytes, 7, tag);
    body := len bytes - at;
    at--;
    bytes[at] = byte (128 + body);
    bytes[7] = kind;
    for (i := 15; i > 7; i--) {
        bytes[i] = byte value;
        value >>= 8;
    }
    crc := checksum(bytes[at:]);
    at--;
    bytes[at] = crc;
    return bytes[at:];
}


# EN: Validate the route tag before constructing a record.
# RU: Проверить тег маршрута перед построением записи.
validate(tag: big)
{
    if (tag <= big 0 || tag > big 16rFFFFFFFF) {
        raise "invalid scalar route tag";
    }
}


# EN: Send one record on a borrowed write FD. No replay after a write failure.
# RU: Отправить одну запись через заимствованный FD записи. Без повтора при ошибке.
send(sys: Sys, fd: ref Sys->FD, tag: big, kind, value: int)
{
    validate(tag);
    if (kind < 1 || kind > 3 || fd == nil || sys == nil) {
        raise "invalid scalar send";
    }
    outgoing := frame(tag, byte kind, big value);
    if (sys->write(fd, outgoing, len outgoing) != len outgoing) {
        raise "remote scalar call outcome unknown";
    }
}


# EN: Receive one checked scalar from a borrowed read FD. Wait only while the
# native inbox reports pending, up to three seconds between provider reads.
# RU: Принять проверенный scalar через заимствованный FD чтения. Ожидать до
# трёх секунд между чтениями, пока нативный приёмник сообщает об ожидании.
take(sys: Sys, fd: ref Sys->FD, tag: big): (int, int)
{
    validate(tag);
    if (fd == nil || sys == nil) {
        raise "invalid scalar receive";
    }
    incoming := array[32] of byte;
    start := sys->millisec();
    n := sys->read(fd, incoming, len incoming);
    while (n < 0 && sys->sprint("%r") == "native input temporarily unavailable" &&
           sys->millisec() - start < 3000) {
        sys->sleep(10);
        n = sys->read(fd, incoming, len incoming);
    }
    if (n < 12 || n > 16) {
        raise "remote scalar reply unavailable";
    }
    result := big 0;
    for (i := n - 8; i < n; i++) {
        result = (result << 8) | big incoming[i];
    }
    kind := int incoming[n-9];
    expected := frame(tag, byte kind, result);
    if (len expected != n || kind < 1 || kind > 3 || big int result != result) {
        raise "invalid scalar reply";
    }
    for (i = 0; i < n; i++) {
        if (expected[i] != incoming[i]) {
            raise "invalid scalar reply";
        }
    }
    return (kind, int result);
}


# EN: Exchange one record using preauthorized VFS names. Both opens precede submission.
# RU: Обменять одну запись по выданным именам VFS. Оба файла открываются до отправки.
call(request, reply: string, requesttag, replytag: big, value: int): int
{
    validate(requesttag);
    validate(replytag);
    sys := load Sys Sys->PATH;
    input := sys->open(reply, Sys->OREAD);
    output := sys->open(request, Sys->OWRITE);
    if (input == nil || output == nil) {
        raise "remote scalar call not sent";
    }
    send(sys, output, requesttag, 1, value);
    (kind, result) := take(sys, input, replytag);
    if (kind == 3) {
        raise "remote scalar owner error";
    }
    if (kind != 2) {
        raise "invalid scalar reply";
    }
    return result;
}
