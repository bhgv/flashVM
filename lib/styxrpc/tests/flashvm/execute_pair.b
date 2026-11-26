# Copyright (c) 2026 bhgv
# EN: Invoke AS callback files from a real Limbo application on the paired mount.
# RU: Вызвать AS-колбэки из приложения Limbo через штатное парное монтирование.
implement Client;
include "sys.m";
include "styxexec.m";

Client: module { init: fn(); };
sys: Sys; # Paired runtime filesystem and scheduler.
execute: Styxexec; # Client for the hidden node service.
Mount: con "/peer"; # Attached FlashVM session.
Large: con 12000; # Larger than one Styx transport write.

include "named_movie_upload.b";


# EN: Check that the named path cannot execute with these borrowed bytes.
# RU: Проверить отказ при вызове пути с этими заимствованными байтами.
rejected(path: string, bytes: array of byte): int
{
    {
        execute->call(Mount, path, bytes);
    } exception error {
    "*" =>
        return 1;
    }
    return 0;
}


# EN: Wait until the active movie publishes the executable path.
# RU: Дождаться публикации исполняемого пути действующим фильмом.
waitnodes()
{
    for (i := 0; i < 100; ++i) {
        (ok, info) := sys->stat(Mount + "/tools/read");
        if (ok == 0) {
            return;
        }
        sys->sleep(10);
    }
    raise "SWF callback files absent";
}


# EN: Check that the service isn't included even in the raw attach-root directory.
# RU: Проверить отсутствие службы даже в исходном списке корня монтирования.
hidden()
{
    fd := sys->open(Mount, Sys->OREAD);
    if (fd == nil) {
        raise "cannot open paired mount directory";
    }
    for (;;) {
        (n, dirs) := sys->dirread(fd);
        if (n < 0) {
            raise "cannot read paired mount directory";
        }
        if (!n) {
            return;
        }
        for (i := 0; i < n; ++i) {
            if (dirs[i].name == ".exec") {
                raise "execute service leaked into ordinary directory listings";
            }
        }
    }
}


# EN: Check r/w permission isolation, one-shot calls, and in-place binary response.
# RU: Проверить раздельные права r/w, однократность и двоичный ответ в том же буфере.
init()
{
    sys = load Sys Sys->PATH;
    execute = load Styxexec Styxexec->PATH;
    if (execute == nil) {
        raise "cannot load Styxexec";
    }
    execute->init();
    start("execute.swf");
    waitnodes();
    hidden();
    if (sys->open(Mount + "/tools/apply", Sys->OREAD) != nil ||
        sys->open(Mount + "/tools/apply", Sys->OWRITE) != nil) {
        raise "execute permission allowed ordinary reads or writes";
    }
    bytes := array[] of {byte 255, byte 40, byte 0, byte 23};
    if (!rejected("tools/read", bytes) || !rejected("tools/fail", bytes) ||
        !rejected("tools/none", bytes) || !rejected("tools/../tools/apply", bytes)) {
        raise "an ineligible path executed successfully";
    }
    if (len execute->call(Mount, "tools/apply", nil)) {
        raise "empty invocation returned bytes";
    }
    result := execute->call(Mount, "tools/apply", bytes);
    if (len result != 3 || result[0] != byte 2 || result[1] != byte 23 || result[2] != byte 255 ||
        bytes[0] != byte 255 || bytes[2] != byte 0) {
        raise "unexpected execute reply or mutation of the caller's input";
    }
    many := array[Large] of byte;
    for (i := 0; i < len many; ++i) {
        many[i] = byte i;
    }
    result = execute->call(Mount, "tools/apply", many);
    if (len result != 3 || result[0] != byte 3 || result[1] != many[Large-1] || result[2] != byte 0) {
        raise "fragmented request did not execute exactly once";
    }
    sys->print("PASS native execute RPC paired VMs\n");
}
