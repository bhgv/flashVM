# Copyright (c) 2026 bhgv
# EN: Access the paired named channel service with independent file descriptors.
# RU: Доступ к именованным каналам пары через независимые дескрипторы.
implement Styxnamed;
include "sys.m";
include "styxnamed.m";

sys: Sys; # Owned module initialized before use.
values: Styxvalue; # Typed codec, optional for callers of the raw interface.
Value: import values;
Open, Read, Write, Newcell, Attachcell, Getcell, Setcell: con iota+1; # Request opcodes.
Header: con 5; # Request header, bytes.
Reply: con 4; # Reply length prefix, bytes.
Maxframe: con 65536; # Maximum payload in one request.
Idlems: con 10; # Sleep between empty polls.
Token: con 24; # Cell capability byte count.
Intrefbytes: con 8; # Legacy Int32 descriptor byte count.


# EN/RU: See styxnamed.m. / См. styxnamed.m.
init()
{
    sys = load Sys Sys->PATH;
    values = load Styxvalue Styxvalue->PATH;
    if (values != nil) {
        values->init();
    }
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


# EN: Open one independent control fid for a channel or live cell.
# RU: Открыть отдельный управляющий fid для канала либо живой ячейки.
control(): ref Sys->FD
{
    fd := sys->open("/.channels", Sys->ORDWR);
    if (fd == nil) {
        raise sys->sprint("cannot open named channel service: %r");
    }
    return fd;
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
open(path: string): ref Port
{
    fd := control();
    submit(fd, Open, array of byte path);
    return ref Port(fd);
}


# EN: Read one nonempty reply frame from the current fid.
# RU: Прочитать один непустой кадр ответа из текущего fid.
readframe(fd: ref Sys->FD): array of byte
{
    size := response(fd);
    if (!size) {
        raise "empty named channel read reply";
    }
    bytes := array[size] of byte;
    at := 0;
    while (at < size) {
        n := sys->read(fd, bytes[at:], size-at);
        if (n <= 0) {
            raise sys->sprint("named channel frame failed: %r");
        }
        at += n;
    }
    return bytes;
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Port.recv(port: self ref Port): array of byte
{
    submit(port.fd, Read, nil);
    return readframe(port.fd);
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


# EN/RU: See styxnamed.m. / См. styxnamed.m.
adopt(frame: array of byte): ref Cell
{
    if (len frame != Token+1 || int frame[0] != Cellref) {
        raise "invalid named cell descriptor";
    }
    fd := control();
    copy := array[len frame] of byte;
    copy[0:] = frame;
    submit(fd, Attachcell, copy[1:]);
    return ref Cell(fd, copy);
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
share(value: array of byte): ref Cell
{
    if (!len value) {
        raise "named cell value is empty";
    }
    fd := control();
    submit(fd, Newcell, value);
    token := readframe(fd);
    if (len token != Token) {
        raise "invalid named cell token";
    }
    frame := array[Token+1] of byte;
    frame[0] = byte Cellref;
    frame[1:] = token;
    return ref Cell(fd, frame);
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Cell.get(cell: self ref Cell): array of byte
{
    submit(cell.fd, Getcell, nil);
    return readframe(cell.fd);
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Cell.set(cell: self ref Cell, value: array of byte)
{
    submit(cell.fd, Setcell, value);
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Cell.payload(cell: self ref Cell): array of byte
{
    if (cell.fd == nil) {
        raise "named cell lease is closed";
    }
    copy := array[len cell.frame] of byte;
    copy[0:] = cell.frame;
    return copy;
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Cell.close(cell: self ref Cell)
{
    cell.fd = nil;
    cell.frame = nil;
}


# EN: Return the initialized codec required by typed operations.
# RU: Выдать инициализированный кодек для типизированных операций.
codec(): Styxvalue
{
    if (values == nil) {
        raise "cannot load styxvalue.dis for typed named channels";
    }
    return values;
}


# EN: Load the exact reference adapter only for Modref traffic.
# RU: Загружать адаптер точных ссылок только при обмене Modref.
refcodec(): Styxmodref
{
    refs := load Styxmodref Styxmodref->PATH;
    if (refs == nil) {
        raise "cannot load styxmodref.dis for named module references";
    }
    refs->init();
    return refs;
}


# EN: Recognize the existing version-one Int32 descriptor before forwarding.
# RU: Распознать существующий дескриптор Int32 версии один перед пересылкой.
intdescriptor(bytes: array of byte): int
{
    return len bytes == Intrefbytes && int bytes[0] == 'V' && int bytes[1] == 'R' &&
        int bytes[2] == 1 && int bytes[3] <= 1 && int bytes[4] >= 128;
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Port.recvvalue(port: self ref Port): ref Message
{
    frame := port.recv();
    case int frame[0] {
    Cellref =>
        return ref Message.Shared(adopt(frame));
    Intref =>
        if (!intdescriptor(frame[1:])) {
            raise "invalid named Int32 descriptor";
        }
        return ref Message.Intrefdata(frame[1:]);
    Moduleref =>
        refs := refcodec();
        return ref Message.Modref(refs->adopt(frame));
    * =>
        v := codec();
        return ref Message.Data(v->decode(frame));
    }
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Port.sendvalue(port: self ref Port, message: ref Message)
{
    if (message == nil) {
        raise "missing named message";
    }
    pick item := message {
    Data =>
        v := codec();
        port.send(v->encode(item.value));
    Shared =>
        if (item.cell == nil) {
            raise "missing named cell lease";
        }
        port.send(item.cell.payload());
    Intrefdata =>
        if (!intdescriptor(item.descriptor)) {
            raise "invalid named Int32 descriptor";
        }
        frame := array[Intrefbytes+1] of byte;
        frame[0] = byte Intref;
        frame[1:] = item.descriptor;
        port.send(frame);
    Modref =>
        if (item.reference == nil) {
            raise "missing named module reference";
        }
        refs := refcodec();
        Cell: import refs;
        port.send(item.reference.payload());
    }
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Cell.getvalue(cell: self ref Cell): ref Value
{
    v := codec();
    return v->decode(cell.get());
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
Cell.setvalue(cell: self ref Cell, value: ref Value)
{
    v := codec();
    cell.set(v->encode(value));
}


# EN/RU: See styxnamed.m. / См. styxnamed.m.
sharevalue(value: ref Value): ref Cell
{
    v := codec();
    return share(v->encode(value));
}
