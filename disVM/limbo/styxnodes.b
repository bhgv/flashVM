# Copyright (c) 2026 bhgv
# EN: Own a local publication and dispatch binary file operations on the Limbo thread.
# RU: Удерживать локальную публикацию и обрабатывать двоичные файловые операции в потоке Limbo.
implement Styxnodes;
include "sys.m";
include "styxnodes.m";

sys: Sys; # Runtime system module loaded by init.
Word: con 4; # Width of an owner mailbox word in bytes.
Replyheader: con 2*Word; # Ticket and confirmed count.
Requestheader: con 3*Word; # Ticket, capacity and operation.
Positionheader: con Requestheader+2*Word; # Adds an unsigned 64-bit byte offset.
Maxoffset: con big 9007199254740991; # Largest exact ActionScript byte offset.


# EN/RU: See styxnodes.m. / См. styxnodes.m.
init()
{
    sys = load Sys Sys->PATH;
}


# EN: Encode the low 32 bits of n in wire at the given byte offset.
# RU: Записать младшие 32 бита n в wire с заданного смещения в байтах.
put(wire: array of byte, at, n: int)
{
    for (i := at+Word-1; i >= at; --i) {
        wire[i] = byte n;
        n >>= 8;
    }
}


# EN: Decode an unsigned mailbox word at the given byte offset in wire.
# RU: Прочитать беззнаковое слово очереди с заданного смещения в байтах.
take(wire: array of byte, at: int): big
{
    value := big 0;
    for (i := at; i < at+Word; ++i) {
        value = (value << 8) | big wire[i];
    }
    return value;
}


# EN: Convert unique ASCII permission letters to the Unix access bits.
# RU: Преобразовать уникальные буквы прав доступа ASCII в биты доступа Unix.
access(permissions: string): int
{
    mode := 0;
    for (i := 0; i < len permissions; ++i) {
        bit: int;
        case permissions[i] {
        'r' => bit = 4;
        'w' => bit = 2;
        'x' => bit = 1;
        * => raise "invalid Styx node permission";
        }
        if (mode & bit) {
            raise "duplicate Styx node permission";
        }
        mode |= bit;
    }
    if (!mode) {
        raise "missing Styx node permissions";
    }
    return mode;
}


# EN: Write one complete registration with optional UInt64 length to the local owner fid.
# RU: Записать полную регистрацию с необязательной длиной UInt64 в локальный fid владельца.
publish_node(path: string, mode: int, length: big): ref Node
{
    name := array of byte path;
    if (len name < 2 || len name > Maxpath || name[0] != byte '/') {
        raise "invalid Styx node publication path";
    }
    prefix := 2;
    if (length >= big 0) {
        mode |= 16r80;
        prefix += 2*Word;
    }
    wire := array[prefix+len name] of byte;
    wire[0] = byte 0;
    wire[1] = byte mode;
    if (prefix > 2) {
        put(wire, 2, int (length >> 32));
        put(wire, 2+Word, int length);
    }
    wire[prefix:] = name;
    fd := sys->open("/.execctl", Sys->ORDWR);
    if (fd == nil || sys->write(fd, wire, len wire) != len wire) {
        raise sys->sprint("cannot publish Styx node: %r");
    }
    return ref Node(fd, array[Positionheader+Maxbytes] of byte, path);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
publish(path, permissions: string): ref Node
{
    return publish_node(path, access(permissions), big -1);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
publishat(path, permissions: string, length: big): ref Node
{
    if (length < big 0 || length > Maxoffset) {
        raise "invalid Styx node length";
    }
    return publish_node(path, access(permissions), length);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
recvat(node: ref Node): (ref Request, big)
{
    fd := node.fd;
    if (fd == nil) {
        raise "Styx node is closed";
    }
    got := sys->read(fd, node.wire, len node.wire);
    while (!got) {
        sys->sleep(10);
        got = sys->read(fd, node.wire, len node.wire);
    }
    if (got < Requestheader) {
        raise sys->sprint("cannot receive Styx node request: %r");
    }
    length := take(node.wire, Word);
    header := big got-length;
    offset := big 0;
    if (header == big Positionheader) {
        offset = (take(node.wire, Requestheader) << 32) | take(node.wire, Requestheader+Word);
    } else if (header != big Requestheader) {
        raise "invalid Styx node request length";
    }
    ticket := take(node.wire, 0);
    operation := int take(node.wire, Replyheader);
    if (operation != 'r' && operation != 'w' && operation != 'x') {
        raise "invalid Styx node operation";
    }
    buffer := array[int length] of byte;
    buffer[0:] = node.wire[int header:got];
    return (ref Request(ticket, sys->sprint("%c", operation), buffer), offset);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
Node.recv(node: self ref Node): ref Request
{
    (item, nil) := recvat(node);
    return item;
}


# EN: Submit one atomic local reply for request with an optional borrowed body.
# RU: Передать один атомарный локальный ответ на request с заимствованным телом.
answer(node: ref Node, request: ref Request, count: int, body: array of byte): int
{
    if (node.fd == nil || request == nil || request.ticket <= big 0 ||
        request.ticket > big 16rffffffff) {
        return 0;
    }
    wire := array[Replyheader+len body] of byte;
    put(wire, 0, int request.ticket);
    put(wire, Word, count);
    wire[Replyheader:] = body;
    return sys->write(node.fd, wire, len wire) == len wire;
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
Node.reply(node: self ref Node, request: ref Request, count: int): int
{
    if (request == nil || count < 0 || count > Maxbytes || count > len request.buffer) {
        return 0;
    }
    body: array of byte;
    if (count && request.operation != "w") {
        body = request.buffer[:count];
    }
    return answer(node, request, count, body);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
Node.refuse(node: self ref Node, request: ref Request): int
{
    return answer(node, request, -1, nil);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
Node.dispatch(node: self ref Node, handler: StyxnodeHandler): int
{
    if (handler == nil) {
        raise "missing Styx node handler";
    }
    item := node.recv();
    count: int;
    {
        count = handler->transfer(node.path, item.operation, item.buffer);
    } exception e {
    "*" =>
        return node.refuse(item);
    }
    if (count < 0 || count > len item.buffer) {
        return node.refuse(item);
    }
    return node.reply(item, count);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
dispatchat(node: ref Node, handler: StyxnodePositionedHandler): int
{
    if (handler == nil) {
        raise "missing positioned Styx node handler";
    }
    (item, offset) := recvat(node);
    count: int;
    {
        count = handler->transfer(node.path, item.operation, item.buffer, offset);
    } exception e {
    "*" =>
        return node.refuse(item);
    }
    if (count < 0 || count > len item.buffer) {
        return node.refuse(item);
    }
    return node.reply(item, count);
}


# EN/RU: See styxnodes.m. / См. styxnodes.m.
Node.close(node: self ref Node)
{
    node.fd = nil;
}
