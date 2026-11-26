# Copyright (c) 2026 bhgv
# EN: Register a Limbo executable node and serve a source-built FlashVM caller.
# RU: Зарегистрировать исполняемый узел Limbo и обслужить вызовы собранного SWF.
implement Client;
include "sys.m";
include "styxexec.m";

Client: module { init: fn(); };
sys: Sys; # Runtime filesystem and scheduler.
exec: Styxexec; # Existing execute client plus local publication.
Node: import exec;
Path: con "/tools/apply"; # Public execute-only fixture path.

include "named_movie_upload.b";


# EN: Confirm an existing ordinary file cannot be replaced by publication.
# RU: Подтвердить, что публикация не заменяет существующий обычный файл.
occupied(path: string): int
{
    {
        bad := exec->publish(path);
        bad.close();
    } exception e {
    "*" =>
        return 1;
    }
    return 0;
}


# EN: Process each request on this Limbo thread and check the binary payload.
# RU: Обработать каждый запрос в этом потоке Limbo и проверить двоичный вход.
serve(node, done: ref Node)
{
    first := node.recv();
    if (len first.bytes || !node.reply(first, nil) || node.refuse(first)) {
        raise "empty invocation or duplicate reply failed";
    }
    handler := load StyxexecHandler "publish-handler.dis";
    if (handler == nil || !node.dispatch(handler) || !node.dispatch(handler)) {
        raise "cannot reply to binary request";
    }
    mark := done.recv();
    if (len mark.bytes != 1 || mark.bytes[0] != byte 207 || !done.reply(mark, nil)) {
        raise "FlashVM did not confirm the received reply";
    }
}


# EN: Publish before startup and observe withdrawal after closing the owner.
# RU: Опубликовать до запуска и проверить отзыв после закрытия владельца.
init()
{
    sys = load Sys Sys->PATH;
    exec = load Styxexec Styxexec->PATH;
    if (exec == nil) {
        raise "cannot load Styxexec";
    }
    exec->init();
    if (!occupied("/publish.swf") || !occupied("/missing/entry")) {
        raise "publication replaced an ordinary file";
    }
    node := exec->publish(Path);
    done := exec->publish("/tools/done");
    (found, info) := sys->stat(Path);
    if (found != 0 || (info.mode & 8r777) != 8r111 || sys->open(Path, Sys->OREAD) != nil ||
        !occupied(Path)) {
        raise "executable marker has incorrect VFS permissions";
    }
    start("publish.swf");
    serve(node, done);
    done.close();
    node.close();
    node.close();
    for (i := 0; i < 100; ++i) {
        (found, info) = sys->stat(Path);
        if (found < 0) {
            sys->print("PASS native publish RPC paired VMs\n");
            return;
        }
        sys->sleep(10);
    }
    raise "closing the Limbo owner did not remove its VFS marker";
}
