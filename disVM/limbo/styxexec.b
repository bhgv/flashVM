# Copyright (c) 2026 bhgv
# EN: Stream one executable callback request and collect its bounded byte reply.
# RU: Передать один запрос к исполняемому колбэку и собрать ограниченный ответ.
implement Styxexec;
include "sys.m";
include "styxexec.m";

sys: Sys; # Borrowed runtime module after init.
Word: con 4; # Big-endian field width in bytes.
Header: con 2*Word; # One count for the pathname and one for the argument.
Service: con ".exec"; # Hidden protocol endpoint, relative to the mount root.
Serviceqid: con big 3; # Exact service identity in the callback tree.


# EN/RU: See styxexec.m. / См. styxexec.m.
init()
{
    sys = load Sys Sys->PATH;
}


# EN: Write a complete header word with the most significant byte first.
# RU: Записать целое слово заголовка старшим байтом вперёд.
put(header: array of byte, at, n: int)
{
    for (i := at+Word-1; i >= at; --i) {
        header[i] = byte n;
        n >>= 8;
    }
}


# EN: Submit exactly the borrowed bytes through the private file cursor.
# RU: Передать все заимствованные байты через курсор отдельного файла.
send(fd: ref Sys->FD, bytes: array of byte)
{
    for (at := 0; at < len bytes;) {
        sent := sys->write(fd, bytes[at:], len bytes-at);
        if (sent <= 0) {
            raise sys->sprint("Styx node execute request failed: %r");
        }
        at += sent;
    }
}


# EN/RU: See styxexec.m. / См. styxexec.m.
call(mount, path: string, input: array of byte): array of byte
{
    name := array of byte path;
    if (!len mount || mount[0] != '/' || !len name || len name > Maxpath ||
        name[0] == byte '/' || len input > Maxbytes) {
        raise "invalid Styx node execute request";
    }
    terminated := array[len name+1] of byte;
    terminated[0:] = name;
    terminated[len name] = byte 0;
    header := array[Header] of byte;
    put(header, 0, len terminated);
    put(header, Word, len input);
    reply := array[len input] of byte;
    if (mount[len mount-1] != '/') {
        mount += "/";
    }
    fd := sys->open(mount + Service, Sys->ORDWR);
    if (fd == nil) {
        raise sys->sprint("cannot open Styx node execute service: %r");
    }
    (ok, info) := sys->fstat(fd);
    if (ok < 0 || info.name != Service || info.qid.qtype != Sys->QTFILE || info.qid.path != Serviceqid) {
        raise "invalid Styx node execute service";
    }
    send(fd, header);
    send(fd, terminated);
    send(fd, input);
    if (len reply && sys->seek(fd, big 0, Sys->SEEKSTART) != big 0) {
        raise sys->sprint("cannot seek to Styx node execute reply: %r");
    }
    at := 0;
    while (at < len reply) {
        got := sys->read(fd, reply[at:], len reply-at);
        if (got < 0) {
            raise sys->sprint("Styx node execute reply failed: %r");
        }
        if (!got) {
            break;
        }
        at += got;
    }
    return reply[:at];
}


# EN: Decode one unsigned owner mailbox word without sign extension.
# RU: Прочитать беззнаковое слово очереди владельца без расширения знака.
take(wire: array of byte, at: int): big
{
    value := big 0;
    for (i := at; i < at+Word; ++i) {
        value = (value << 8) | big wire[i];
    }
    return value;
}


# EN/RU: See styxexec.m. / См. styxexec.m.
publish(path: string): ref Node
{
    bytes := array of byte path;
    if (len bytes < 2 || len bytes >= Maxpath || bytes[0] != byte '/') {
        raise "invalid executable publication path";
    }
    node := ref Node(nil, array[Header+Maxbytes] of byte, path);
    node.fd = sys->open("/.execctl", Sys->ORDWR);
    if (node.fd == nil || sys->write(node.fd, bytes, len bytes) != len bytes) {
        raise sys->sprint("cannot publish executable node: %r");
    }
    return node;
}


# EN/RU: See styxexec.m. / См. styxexec.m.
Node.recv(node: self ref Node): ref Request
{
    fd := node.fd;
    if (fd == nil) {
        raise "executable node is closed";
    }
    got := sys->read(fd, node.wire, len node.wire);
    while (!got) {
        sys->sleep(10);
        got = sys->read(fd, node.wire, len node.wire);
    }
    if (got < Header || big (got-Header) != take(node.wire, Word)) {
        raise sys->sprint("cannot receive executable request: %r");
    }
    ticket := take(node.wire, 0);
    bytes := array[got-Header] of byte;
    bytes[0:] = node.wire[Header:got];
    return ref Request(ticket, bytes);
}


# EN: Submit one whole local response by exact ticket. A stale request fails safely.
# RU: Передать целый локальный ответ с точным билетом. Истёкший запрос отклоняется.
answer(node: ref Node, request: ref Request, bytes: array of byte, count: int): int
{
    if (node.fd == nil || request == nil || request.ticket <= big 0 ||
        request.ticket > big 16rffffffff || len bytes > len request.bytes) {
        return 0;
    }
    wire := array[Header+len bytes] of byte;
    put(wire, 0, int request.ticket);
    put(wire, Word, count);
    wire[Header:] = bytes;
    return sys->write(node.fd, wire, len wire) == len wire;
}


# EN/RU: See styxexec.m. / См. styxexec.m.
Node.reply(node: self ref Node, request: ref Request, bytes: array of byte): int
{
    return answer(node, request, bytes, len bytes);
}


# EN/RU: See styxexec.m. / См. styxexec.m.
Node.refuse(node: self ref Node, request: ref Request): int
{
    return answer(node, request, nil, -1);
}


# EN/RU: See styxexec.m. / См. styxexec.m.
Node.dispatch(node: self ref Node, handler: StyxexecHandler): int
{
    if (handler == nil) {
        raise "missing executable node handler";
    }
    item := node.recv();
    response: array of byte;
    {
        response = handler->invoke(node.path, item.bytes);
    } exception e {
    "*" =>
        return node.refuse(item);
    }
    if (len response > len item.bytes) {
        return node.refuse(item);
    }
    return node.reply(item, response);
}


# EN/RU: See styxexec.m. / См. styxexec.m.
Node.close(node: self ref Node)
{
    node.fd = nil;
}
