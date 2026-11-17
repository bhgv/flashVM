# Copyright (c) 2026 bhgv
# EN: Generic provider bootstrap prepares environment, then calls a typed application init.
# RU: Общий provider bootstrap готовит environment и вызывает typed application init.
implement Check;
include "terminalcmd.m";
Check: module { init: fn(); };


# EN/RU: Reject integration mismatch explicitly, independent of build mode.
# Явно отклонить integration mismatch независимо от build mode.
check(ok: int, message: string)
{
    if (!ok) {
        raise message;
    }
}


# EN/RU: Load interfaces on provider, prepare namespace, capture channels, unmount path and invoke init.
# Загрузить interfaces на provider, подготовить namespace, capture channels и вызвать init после unmount.
init()
{
    sys := load Sys Sys->PATH;
    api := load Vfsexport Vfsexport->PATH;
    term := load Terminal Terminal->PATH;
    app := load Terminalcmd "app.dis";
    check(api != nil && term != nil && app != nil, "provider interfaces");
    term->init();
    check(term->current() == nil && api->terminal() == nil && api->terminalmovie() == nil && api->terminalpath(0) == nil,
        "ordinary startup invented a launch grant");
    check(sys->pctl(Sys->NEWNS, nil) > 0, "prepare environment before application init");
    root := api->capture("/link", 0, 7);
    readonly := api->capture("/link", 0, 4);
    check(root != nil && readonly != nil && sys->unmount(nil, "/link") == 0, "capture channel grant");
    check(sys->open("/link/tx", Sys->OWRITE) == nil, "channel path survived unmount");
    check(api->open(root, "../tx", Sys->OWRITE) == nil && api->open(root, "/tx", Sys->OWRITE) == nil &&
        api->open(root, "tx", Sys->OWRITE | Sys->OTRUNC) == nil &&
        api->open(root, nil, Sys->OREAD) == nil, "capability path/mode escape");
    check(api->open(readonly, "tx", Sys->OWRITE) == nil && term->connect(readonly, "rx", "tx") == nil,
        "channel grant rights bypass");
    duplex := term->connect(root, "ctl", "ctl");
    check(duplex != nil && duplex.receive == duplex.send && duplex.movie == nil && term->movies(duplex) == nil, "duplex opened twice");
    check(sys->write(duplex.send, array of byte "x", 1) == 1, "duplex write");
    bytebuf := array[1] of byte;
    check(sys->read(duplex.receive, bytebuf, 1) == 1 && string bytebuf == "x", "duplex read");
    session := term->connect(root, "rx", "tx");
    check(session != nil && session.receive != session.send, "paired channels");
    app->init(session, "on-provider" :: nil);
    extra := term->open(session, "streams/17", Sys->ORDWR);
    check(extra != nil, "generic extra channel helper");
    for (i := 0; i < 12; i++) {
        extra = term->open(session, "streams/17", Sys->ORDWR);
        check(extra != nil, "repeated FD result ownership");
    }
    extra = api->open(nil, "tx", Sys->OWRITE);
    check(extra == nil, "nil root retained old FD result");
    sys->print("PASS terminal capability survives namespace removal\n");
}
