# Copyright (c) 2026 bhgv
# EN: Access the paired named channel service with independent file descriptors.
# RU: Доступ к именованным каналам пары через независимые дескрипторы.
implement Styxnamed;
include "sys.m";
include "styxnamed.m";

sys: Sys; # Owned module initialized before use.
Open, Read, Write: con iota+1; # Request opcodes.
Header: con 5; # Request header, bytes.
Reply: con 4; # Reply length prefix, bytes.
Maxframe: con 65536; # Maximum payload in one request.
Idlems: con 10; # Sleep between empty polls.


# EN/RU: See styxnamed.m. / См. styxnamed.m.
init()
{
    sys = load Sys Sys->PATH;
}


# EN: Write an entire borrowed packet, observing short confirmed writes.
# RU: Записать весь заимствованный кадр с учётом подтверждённых коротких записей.
writeall(fd: ref Sys->FD, bytes: array of byte)
{
    at := 0;
    while (at < len bytes) {
        n := sys->write(fd, bytes[at:], len bytes-at);
        if (n <= 0) {
            raise sys->sprint("named channel request failed: %r");
        }
        at += n;
    }
}


# EN: Start a new request at exact position zero and transmit its optional body.
# RU: Начать новый запрос в позиции ноль и передать необязательное тело.
submit(fd: ref Sys->FD, operation: int, body: array of byte)
{
    if (fd == nil || len body > Maxframe || sys->seek(fd, big 0, Sys->SEEKSTART) != big 0) {
        raise "invalid named channel request";
    }
    size := len body;
    header := array[Header] of byte;
    header[0] = byte operation;
    for (i := Header-1; i > 0; --i) {
        header[i] = byte size;
        size >>= 8;
    }
    writeall(fd, header);
    writeall(fd, body);
}


# EN: Read a complete acknowledgement length, sleeping only while waiting to pair.
# RU: Прочитать длину ответа целиком, ожидая только до сопоставления с другой операцией.
response(fd: ref Sys->FD): int
{
    if (sys->seek(fd, big 0, Sys->SEEKSTART) != big 0) {
        raise sys->sprint("seek named channel reply: %r");
    }
    prefix := array[Reply] of byte;
    at := 0;
    while (at < Reply) {
        n := sys->read(fd, prefix[at:], Reply-at);
        if (n < 0 || (n == 0 && at != 0)) {
            raise sys->sprint("named channel reply failed: %r");
        }
        if (!n) {
            sys->sleep(Idlems);
        }
        at += n;
    }
    size := 0;
    for (i := 0; i < Reply; ++i) {
        size = (size << 8) | int prefix[i];
    }
    if (size < 0 || size > Maxframe) {
        raise "invalid named channel reply length";
    }
    return size;
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
open(path: string): ref Port
{
    fd := sys->open("/.channels", Sys->ORDWR);
    if (fd == nil) {
        raise sys->sprint("cannot open named channel service: %r");
    }
    submit(fd, Open, array of byte path);
    return ref Port(fd);
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Port.recv(port: self ref Port): array of byte
{
    submit(port.fd, Read, nil);
    size := response(port.fd);
    if (!size) {
        raise "empty named channel read reply";
    }
    bytes := array[size] of byte;
    at := 0;
    while (at < size) {
        n := sys->read(port.fd, bytes[at:], size-at);
        if (n <= 0) {
            raise sys->sprint("named channel frame failed: %r");
        }
        at += n;
    }
    return bytes;
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Port.send(port: self ref Port, frame: array of byte)
{
    if (!len frame) {
        raise "named channel frame is empty";
    }
    submit(port.fd, Write, frame);
    if (response(port.fd)) {
        raise "nonempty named channel write reply";
    }
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Port.close(port: self ref Port)
{
    port.fd = nil;
}
