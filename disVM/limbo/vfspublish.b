# Copyright (c) 2026 bhgv
# EN: One-request UTF-8 publication reader; no native polling loop or foreign guest callback.
# RU: One-request UTF-8 publication reader; без native polling loop и foreign guest callback.
implement Vfspublish;
include "sys.m";
include "vfspublish.m";
api: Vfsexport; # Immutable loaded builtin after init.


# EN/RU: Initialize before sharing this library; fail explicitly if host lacks the extension.
# Инициализировать до sharing library; явно отказать, если host не имеет extension.
init()
{
    api = load Vfsexport Vfsexport->PATH;
    if (api == nil) {
        raise "Vfspublish requires Vfsexport";
    }
}


# EN/RU: Take one request and copy its immutable name, bounded by native path capacity.
# Получить один request, скопировать immutable name; native capacity ограничивает длину.
next(owner: ref Vfsexport->Owner): ref Request
{
    handle := api->next(owner);
    if (handle == nil) {
        return nil;
    }
    (id, pid, kind, rights) := api->describe(handle);
    bytes := array[4096] of byte;
    for (i := 0; i < len bytes; i++) {
        (status, value) := api->text(handle, i);
        if (status == 0) {
            return ref Request(handle, id, pid, kind, rights, string bytes[:i]);
        }
        if (status != 1 || value < 1 || value > 255) {
            raise "Vfspublish invalid request name";
        }
        bytes[i] = byte value;
    }
    raise "Vfspublish oversized request name";
    return nil;
}


# EN/RU: Native SDK validates exact owner/card publication and limits granted rights.
# Native SDK проверяет exact owner/card publication и ограничивает granted rights.
reply(owner: ref Vfsexport->Owner, request: ref Request, chosen: ref Vfsexport->Root): int
{
    if (request == nil) {
        return 0;
    }
    return api->reply(owner, request.handle, chosen);
}
