# Copyright (c) 2026 bhgv
# EN: Resolve transported descriptors using the issuer mount and exact position zero.
# RU: Разрешать дескрипторы в точке подключения владельца и работать со смещения ноль.
implement Styxintref;
include "sys.m";
include "styxintref.m";
sys: Sys; # Owned immutable Sys module after initialization.


# EN/RU: See styxintref.m. / См. styxintref.m.
init()
{
    sys = load Sys Sys->PATH;
}


# EN/RU: See styxintref.m. / См. styxintref.m.
accept(root: string, bytes: array of byte): ref Cell
{
    if (sys == nil || root == nil || root[0] != '/' || len bytes != 8 ||
        int bytes[0] != 'V' || int bytes[1] != 'R' || int bytes[2] != 1 ||
        int bytes[3] > 1 || int bytes[4] < 128) {
        raise "invalid StyxRPC Int32 reference";
    }
    pid := big 0;
    for (i := 4; i < 8; ++i) {
        pid = (pid << 8) | big bytes[i];
    }
    mode := Sys->OREAD;
    if (int bytes[3] == 1) {
        mode = Sys->ORDWR;
    }
    sep := "/";
    if (root[len root-1] == '/') {
        sep = "";
    }
    fd := sys->open(root + sep + sys->sprint(".prog/%bd/.var/ref", pid), mode);
    if (fd == nil) {
        raise sys->sprint("open StyxRPC Int32 reference: %r");
    }
    (ok, meta) := sys->fstat(fd);
    if (ok < 0 || meta.length != big 4 || ((meta.qid.path >> 32) & big 16rffffffff) != pid) {
        raise "StyxRPC Int32 reference identity mismatch";
    }
    gate := chan[1] of int;
    gate <-= 1;
    wire := array[len bytes] of byte;
    wire[0:] = bytes;
    return ref Cell(fd, gate, root, wire, int bytes[3]);
}


# EN: Copy the original descriptor for forwarding on the same issuer session.
# RU: Скопировать оставшийся дескриптор для пересылки в сессии владельца.
Cell.payload(cell: self ref Cell): array of byte
{
    wire := array[len cell.wire] of byte;
    wire[0:] = cell.wire;
    return wire;
}


# EN: Read one exact signed value through the retained fid.
# RU: Прочитать точное знаковое значение через удерживаемый fid.
Cell.read(cell: self ref Cell): int
{
    bytes := array[4] of byte;
    token := <-cell.gate;
    count := -1;
    if (sys->seek(cell.fd, big 0, Sys->SEEKSTART) == big 0) {
        count = sys->read(cell.fd, bytes, len bytes);
    }
    cell.gate <-= token;
    if (count != len bytes) {
        raise sys->sprint("read StyxRPC Int32 reference: %r");
    }
    value := int bytes[0];
    if (value >= 128) {
        value -= 256;
    }
    for (i := 1; i < len bytes; ++i) {
        value = value * 256 + int bytes[i];
    }
    return value;
}


# EN: Replace the complete value once at offset zero.
# RU: Однократно заменить всё значение со смещения ноль.
Cell.write(cell: self ref Cell, value: int)
{
    if (!cell.writable) {
        raise "read-only StyxRPC Int32 reference";
    }
    bytes := array[] of {byte (value >> 24), byte (value >> 16), byte (value >> 8), byte value};
    token := <-cell.gate;
    count := -1;
    if (sys->seek(cell.fd, big 0, Sys->SEEKSTART) == big 0) {
        count = sys->write(cell.fd, bytes, len bytes);
    }
    cell.gate <-= token;
    if (count != len bytes) {
        raise sys->sprint("write StyxRPC Int32 reference: %r");
    }
}


# EN/RU: See styxintref.m. / См. styxintref.m.
channel(root: string, wire: chan of array of byte): ref Channel
{
    if (root == nil || root[0] != '/' || wire == nil) {
        raise "invalid StyxRPC reference channel";
    }
    return ref Channel(root, wire);
}


# EN: Send an existing reference on its declared issuer mount.
# RU: Отправить выданную ссылку в объявленном подключении владельца.
Channel.send(port: self ref Channel, cell: ref Cell)
{
    if (cell == nil || cell.root != port.root) {
        raise "StyxRPC reference channel issuer mismatch";
    }
    port.wire <-= cell.payload();
}


# EN: Receive and immediately bind the next issued reference.
# RU: Принять и сразу привязать следующую выданную ссылку.
Channel.recv(port: self ref Channel): ref Cell
{
    return accept(port.root, <-port.wire);
}
