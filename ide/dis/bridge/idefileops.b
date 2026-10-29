implement Idefileops;
include "sys.m";
include "idefileops.m";


# Validate a borrowed native path before any operation can open or truncate it.
# Returns one for nonempty, bounded, NUL-free paths; no filesystem sandbox is implied.
validpath(path: string): int
{
    if (path == "" || len array of byte path > 4096) {
        return 0;
    }
    for (i := 0; i < len path; i++) {
        if (path[i] == 0) {
            return 0;
        }
    }
    return 1;
}


# Read a bounded strict UTF-8 snapshot; ownership/status/blocking follow idefileops.m.
read(sys: Sys, path: string, limit: int): (int, string)
{
    (status, bytes) := readbytes(sys, path, limit);
    if (status != Ok) {
        return (status, "");
    }
    count := len bytes;
    text := string bytes[:count];
    encoded := array of byte text;
    if (len encoded != count) {
        return (Invalid, "");
    }
    for (i := 0; i < count; i++) {
        if (encoded[i] != bytes[i]) {
            return (Invalid, "");
        }
    }
    return (Ok, text);
}


# Read binary data without UTF-8 conversion; see the public bounded I/O contract.
# sys/path are borrowed; failure closes the local descriptor and returns nil.
readbytes(sys: Sys, path: string, limit: int): (int, array of byte)
{
    if (sys == nil || !validpath(path) || limit < 1 || limit > MaxBytes) {
        return (Invalid, nil);
    }
    fd := sys->open(path, Sys->OREAD);
    if (fd == nil) {
        return (Io, nil);
    }
    bytes := array[limit + 1] of byte;
    count := 0;
    while (count < len bytes) {
        n := sys->read(fd, bytes[count:], len bytes - count);
        if (n < 0) {
            return (Io, nil);
        }
        if (n == 0) {
            break;
        }
        count += n;
    }
    if (count > limit) {
        return (TooLarge, nil);
    }
    return (Ok, bytes[:count]);
}



# Validate then completely write a borrowed snapshot; no descriptor/array escapes.
# Status and failure-side-effect guarantees follow the public interface.
write(sys: Sys, path: string, text: string): int
{
    if (sys == nil || !validpath(path)) {
        return Invalid;
    }
    return writebytes(sys, path, array of byte text, 0);
}


# Completely write borrowed immutable bytes; validation precedes creation and
# exclusive admission is performed by the OS port, never by a racy existence test.
writebytes(sys: Sys, path: string, bytes: array of byte, exclusive: int): int
{
    if (sys == nil || !validpath(path) || (exclusive != 0 && exclusive != 1)) {
        return Invalid;
    }
    if (len bytes > MaxBytes) {
        return TooLarge;
    }
    mode := Sys->OWRITE;
    if (exclusive) {
        mode |= Sys->OEXCL;
    }
    fd := sys->create(path, mode, 8r666);
    if (fd == nil) {
        return Io;
    }
    count := 0;
    while (count < len bytes) {
        n := sys->write(fd, bytes[count:], len bytes - count);
        if (n <= 0) {
            return Io;
        }
        count += n;
    }
    return Ok;
}
