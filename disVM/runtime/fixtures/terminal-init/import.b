# Copyright (c) 2026 bhgv
# EN: Native-adopted environment/channel grant is present before the first bootstrap instruction.
# RU: Native-adopted environment/channel grant существует до первой инструкции bootstrap.
implement Check;
include "terminalcmd.m";
include "processctl.m";
Check: module { init: fn(); };


# EN/RU: Fail real interpreter/JIT integration mismatch explicitly.
# Явно завершить real interpreter/JIT integration mismatch.
check(ok: int, message: string)
{
    if (!ok) {
        raise message;
    }
}


# EN/RU: A permanent child owner discovers its inherited grant without namespace lookup.
# Отдельный постоянный child owner получает inherited grant без namespace lookup.
inherited(term: Terminal, ready: chan of int)
{
    session := term->current();
    check(session != nil && session.root != nil && session.receive != nil && session.send != nil,
        "child lost inherited launch grant");
    ctl := load Processctl Processctl->PATH;
    ready <-= ctl->pid();
    never := chan of int;
    <-never;
}


# EN/RU: Load provider-local interfaces through an explicit package grant; then remove every mount.
# Module init receives current terminal directly, with no mount/capture of the channel root.
# Загрузить provider interfaces по package grant, снять все mount; init получает current terminal.
init()
{
    sys := load Sys Sys->PATH;
    api := load Vfsexport Vfsexport->PATH;
    ctl := load Processctl Processctl->PATH;
    check(sys->open("/server", Sys->OREAD) != nil && sys->open("/etc/passwd", Sys->OREAD) == nil,
        "prepared environment missing at first instruction");
    term := load Terminal "/pkg/terminal.dis";
    app := load Terminalcmd "/pkg/app.dis";
    check(term != nil && app != nil, "explicit provider-local package grant");
    term->init();
    check(sys->unmount(nil, "/pkg") == 0 && sys->unmount(nil, "/") == 0 &&
        sys->open("/pkg/terminal.dis", Sys->OREAD) == nil && sys->open("/etc/passwd", Sys->OREAD) == nil,
        "removing all roots exposed fallback");
    if (api->terminal() == nil) {
        check(term->current() == nil && api->terminalpath(0) == nil && api->terminalpath(1) == nil,
            "absent launch invented channels");
        sys->print("PASS launch import without terminal\n");
        return;
    }
    check(api->terminalpath(-1) == nil && api->terminalpath(2) == nil, "invalid channel direction");
    session := term->current();
    check(session != nil && (session.receive == session.send) == (api->terminalpath(0) == api->terminalpath(1)),
        "current terminal duplex/pair association");
    ready := chan of int;
    spawn inherited(term, ready);
    pid := <-ready;
    check(pid > 0 && ctl->killwait(pid) == Processctl->Retired, "inherited owner retirement");
    app->init(session, "on-provider" :: nil);
    sys->print("PASS native launch import before typed application init\n");
}
