# Copyright (c) 2026 bhgv
# EN: Use an SDK-owned export fid from Limbo, with no mount path or cwd dependency.
# RU: Работа с удерживаемым SDK экспортом из Limbo независимо от пути маунта и cwd.
implement Styxmodref;
include "sys.m";
include "styxmodref.m";

sys: Sys; # System operations loaded once before concurrent use.
Capture, Adopt, Capturelocal: con iota+1; # First-write commands from styxrpc/module_ref_port.h.
Frameshift: con 4; # Frame reads start after the four-byte signed scalar.
Maxpath: con 1024; # Maximum counted path bytes.
Maxframe: con 1+28+Maxpath; # Named tag and SDK module reference wire capacity.


# EN/RU: See styxmodref.m. / См. styxmodref.m.
init()
{
    sys = load Sys Sys->PATH;
}


# EN: Submit one complete initial command on an independent binding descriptor.
# RU: Передать полную начальную команду через независимый дескриптор привязки.
bindref(command: array of byte): ref Cell
{
    if (sys == nil) {
        raise "initialize Styxmodref before opening references";
    }
    fd := sys->open("/.varref", Sys->ORDWR);
    if (fd == nil || sys->write(fd, command, len command) != len command) {
        raise sys->sprint("open paired module reference: %r");
    }
    return ref Cell(fd);
}


# EN: Bind the exact path with the selected owner's capture command.
# RU: Привязать точный путь командой захвата выбранного владельца.
captureat(path: string, writable, operation: int): ref Cell
{
    if (path == nil || writable < 0 || writable > 1) {
        raise "invalid module export path or mode";
    }
    bytes := array of byte path;
    if (len bytes > Maxpath) {
        raise "module export path is too long";
    }
    command := array[2+len bytes] of byte;
    command[0] = byte operation;
    command[1] = byte writable;
    command[2:] = bytes;
    return bindref(command);
}


# EN/RU: See styxmodref.m. / См. styxmodref.m.
capture(path: string, writable: int): ref Cell
{
    return captureat(path, writable, Capture);
}


# EN/RU: See styxmodref.m. / См. styxmodref.m.
capturelocal(path: string, writable: int): ref Cell
{
    return captureat(path, writable, Capturelocal);
}


# EN/RU: See styxmodref.m. / См. styxmodref.m.
adopt(frame: array of byte): ref Cell
{
    if (len frame == 0 || len frame > Maxframe) {
        raise "invalid module reference frame";
    }
    command := array[1+len frame] of byte;
    command[0] = byte Adopt;
    command[1:] = frame;
    return bindref(command);
}


# EN: Select the value or payload position. Serialize all operations on one cell.
# RU: Выбрать позицию значения или дескриптора. Операции одного cell выполняются последовательно.
position(cell: ref Cell, offset: big): ref Sys->FD
{
    if (cell == nil || cell.fd == nil || sys->seek(cell.fd, offset, Sys->SEEKSTART) != offset) {
        raise "module reference file is closed or unavailable";
    }
    return cell.fd;
}


# EN/RU: See styxmodref.m. / См. styxmodref.m.
Cell.read(cell: self ref Cell): int
{
    fd := position(cell, big 0);
    bytes := array[4] of byte;
    if (sys->read(fd, bytes, len bytes) != len bytes) {
        raise sys->sprint("read paired module reference: %r");
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


# EN/RU: See styxmodref.m. / См. styxmodref.m.
Cell.write(cell: self ref Cell, value: int)
{
    fd := position(cell, big 0);
    bytes := array[] of {byte (value >> 24), byte (value >> 16), byte (value >> 8), byte value};
    if (sys->write(fd, bytes, len bytes) != len bytes) {
        raise sys->sprint("write paired module reference: %r");
    }
}


# EN/RU: See styxmodref.m. / См. styxmodref.m.
Cell.payload(cell: self ref Cell): array of byte
{
    fd := position(cell, big Frameshift);
    bytes := array[Maxframe] of byte;
    size := sys->read(fd, bytes, len bytes);
    if (size <= 0) {
        raise sys->sprint("read paired module reference frame: %r");
    }
    return bytes[:size];
}


# EN/RU: See styxmodref.m. / См. styxmodref.m.
Cell.close(cell: self ref Cell)
{
    cell.fd = nil;
}
