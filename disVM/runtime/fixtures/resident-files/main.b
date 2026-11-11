# Copyright (c) 2026 bhgv
# EN: Reserve private file/directory slots concurrently and retain all owners.
# RU: Параллельно занимать слоты файлов/каталогов и удерживать всех владельцев.
implement ResidentFiles;
include "sys.m";

ResidentFiles: module { init: fn(); };
sys: Sys; # Immutable builtin interface, initialized before spawn.
ReadMode: con 0; # Driver selects direct write, read or formatted write.
Workers: con 16; # Simultaneously live owners, below the native slot quota.


# EN: Roll back a failed open, then hold a file/directory until parent releases it.
# id selects a private path; ready reports the live slot, done exact I/O completion.
# RU: Откатить неудачное открытие и держать файл/каталог до разрешения родителя.
# id задаёт личный путь; ready сообщает живой слот, done — завершение I/O.
worker(id: int, ready, go, done: chan of int)
{
    missing := sys->open("missing-" + string id, Sys->OREAD);
    if (missing != nil) {
        raise "missing file unexpectedly opened";
    }
    fd: ref Sys->FD;
    if (id % 2) {
        fd = sys->open("dir-" + string id, Sys->OREAD);
    } else {
        if (ReadMode == 1) {
            fd = sys->open("file-" + string id, Sys->OREAD);
        } else {
            fd = sys->create("file-" + string id, Sys->OWRITE, 8r666);
        }
    }
    if (fd == nil) {
        raise "concurrent file slot publication failed";
    }
    ready <-= fd.fd;
    if (<-go != 1) {
        raise "file owner start message changed";
    }
    if (!(id % 2)) {
        bytes := array[64] of { * => byte (65 + id) };
        if (ReadMode == 1) {
            bytes = array[64] of byte;
            if (sys->readn(fd, bytes, len bytes) != len bytes) {
                raise "private file read count changed";
            }
            for (at := 0; at < len bytes; at++) {
                if (int bytes[at] != 65 + id) {
                    raise "private read bytes changed";
                }
            }
        } else {
            if (ReadMode == 2) {
                if (sys->fprint(fd, "%s", string bytes) != len bytes) {
                    raise "private formatted file output changed";
                }
            } else {
                if (sys->write(fd, bytes, len bytes) != len bytes) {
                    raise "private file output changed";
                }
            }
        }
        if (sys->seek(fd, big 0, Sys->SEEKRELA) != big 64) {
            raise "private file cursor changed";
        }
    }
    (status, info) := sys->fstat(fd);
    if (status != 0 || (!(id % 2) && info.length != big 64)) {
        raise "concurrent descriptor metadata changed";
    }
    done <-= 1;
    if (<-go != 2) {
        raise "file owner retirement message changed";
    }
    fd = nil;
}


# EN: Require unique live slots before any owner may retire, then release all.
# RU: Проверить уникальность живых слотов до завершения владельцев и отпустить всех.
init()
{
    sys = load Sys Sys->PATH;
    ready := chan of int;
    go := chan of int;
    done := chan of int;
    seen := array[32] of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(id, ready, go, done);
    }
    for (id = 0; id < Workers; id++) {
        slot := <-ready;
        if (slot < 3 || slot >= len seen || seen[slot]) {
            raise "duplicate live native descriptor slot";
        }
        seen[slot] = 1;
    }
    # Keep every remaining native slot live; quota rejection must precede open.
    # Держать все оставшиеся слоты; отказ квоты должен предшествовать open.
    extra := array[13] of ref Sys->FD;
    for (id = 0; id < len extra; id++) {
        extra[id] = sys->open("dir-1", Sys->OREAD);
        if (extra[id] == nil || extra[id].fd < 3 || extra[id].fd >= len seen || seen[extra[id].fd]) {
            raise "native slot quota or publication changed";
        }
        seen[extra[id].fd] = 1;
    }
    if (sys->create("quota-must-not-create", Sys->OWRITE, 8r666) != nil) {
        raise "native slot quota admitted a thirtieth owner";
    }
    for (id = 0; id < Workers; id++) {
        go <-= 1;
    }
    for (id = 0; id < Workers; id++) {
        if (<-done != 1) {
            raise "file owner completion changed";
        }
    }
    extra = nil;
    for (id = 0; id < Workers; id++) {
        go <-= 2;
    }
    sys->print("PASS resident files owners=16 files=8 directories=8 rollback=16\n");
}
