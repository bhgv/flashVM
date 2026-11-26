# Copyright (c) 2026 bhgv
# EN: Upload a project-compiled fixture through the paired player's control files.
# RU: Загрузить собранный средствами проекта SWF через управляющие файлы плеера.

# EN: Upload the compiled SWF to the paired player's control endpoint and start it.
# RU: Загрузить собранный SWF в точку управления плеера и запустить его.
start(path: string)
{
    fd := sys->open(path, Sys->OREAD);
    (ok, info) := sys->stat(path);
    if (fd == nil || ok < 0 || info.length <= big 0 || info.length > big 65535) {
        raise "cannot size named SWF";
    }
    n := int info.length;
    bytes := array[n+4] of byte;
    at := 4;
    while (at < len bytes) {
        got := sys->read(fd, bytes[at:], len bytes-at);
        if (got <= 0) {
            raise "short read of named SWF";
        }
        at += got;
    }
    bytes[2] = byte (n >> 8);
    bytes[3] = byte n;
    upload := sys->open("/peer/.upload", Sys->OWRITE);
    if (upload == nil) {
        raise sys->sprint("cannot open named SWF upload: %r");
    }
    offset := 0;
    while (offset < n+4) {
        count := n+4-offset;
        if (count > 2048) {
            count = 2048;
        }
        written := sys->write(upload, bytes[offset:], count);
        if (written <= 0) {
            raise sys->sprint("cannot upload named SWF at %d: %r", offset);
        }
        offset += written;
    }
    upload = nil;
    state := sys->open("/peer/.state", Sys->OWRITE);
    command := array of byte "start";
    if (state == nil || sys->write(state, command, len command) != len command) {
        raise sys->sprint("cannot start named SWF: %r");
    }
}
