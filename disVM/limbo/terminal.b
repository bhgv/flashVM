# Copyright (c) 2026 bhgv
# EN: Reuse ordinary capability FD operations for local/remote FlashVM channels.
# RU: Обычные capability FD operations для локальных/удалённых каналов FlashVM.
implement Terminal;
include "terminal.m";
api: Vfsexport; # Immutable after init; no native/foreign heap pointers.


# EN/RU: Load matched builtin before sharing this module; raise if unavailable.
# Загрузить matching builtin до sharing модуля; отсутствие — exception.
init()
{
    api = load Vfsexport Vfsexport->PATH;
    if (api == nil) {
        raise "Terminal requires Vfsexport";
    }
}


# EN/RU: Obtain the current process's immutable launch association and open its control channel(s).
# Получить immutable launch association своего process и открыть control channel(s); session/nil.
current(): ref Session
{
    root := api->terminal();
    if (root == nil) {
        return nil;
    }
    session := connect(root, api->terminalpath(0), api->terminalpath(1));
    if (session != nil) {
        session.movie = api->terminalmovie();
        if (session.movie == nil) {
            return nil;
        }
    }
    return session;
}


# EN/RU: Borrow root/names; return owned session/nil. Same name opens one duplex FD.
# Root/names borrowed; owned session/nil, одинаковое имя открывает один duplex FD.
connect(root: ref Vfsexport->Root, receive, send: string): ref Session
{
    if (root == nil) {
        return nil;
    }
    if (receive == send) {
        fd := api->open(root, receive, Sys->ORDWR);
        if (fd == nil) {
            return nil;
        }
        return ref Session(root, fd, fd, nil);
    }
    input := api->open(root, receive, Sys->OREAD);
    if (input == nil) {
        return nil;
    }
    output := api->open(root, send, Sys->OWRITE);
    if (output == nil) {
        return nil;
    }
    return ref Session(root, input, output, nil);
}


# EN/RU: Borrow session/name/mode; return independent ordinary FD/nil without namespace lookup.
# Session/name/mode borrowed; independent FD/nil, namespace lookup отсутствует.
open(session: ref Session, name: string, mode: int): ref Sys->FD
{
    if (session == nil) {
        return nil;
    }
    return api->open(session.root, name, mode);
}


# EN/RU: Borrow the session, return an independent owned duplex protocol FD or nil.
# Session borrowed; вернуть independent owned duplex protocol FD либо nil.
movies(session: ref Session): ref Sys->FD
{
    if (session == nil || session.movie == nil) {
        return nil;
    }
    return api->open(session.movie, ".", Sys->ORDWR);
}
