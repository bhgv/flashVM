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
    if (sys == nil || !validpath(path) || limit < 1 || limit > MaxBytes) {
        return (Invalid, "");
    }
    fd := sys->open(path, Sys->OREAD);
    if (fd == nil) {
        return (Io, "");
    }
    bytes := array[limit + 1] of byte;
    count := 0;
    while (count < len bytes) {
        n := sys->read(fd, bytes[count:], len bytes - count);
        if (n < 0) {
            return (Io, "");
        }
        if (n == 0) {
            break;
        }
        count += n;
    }
    if (count > limit) {
        return (TooLarge, "");
    }
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


# Validate then completely write a borrowed snapshot; no descriptor/array escapes.
# Status and failure-side-effect guarantees follow the public interface.
write(sys: Sys, path: string, text: string): int
{
    if (sys == nil || !validpath(path)) {
        return Invalid;
    }
    bytes := array of byte text;
    if (len bytes > MaxBytes) {
        return TooLarge;
    }
    fd := sys->create(path, Sys->OWRITE, 8r666);
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
