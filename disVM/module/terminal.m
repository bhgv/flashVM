# Copyright (c) 2026 bhgv
# EN: Location-independent FlashVM byte channels and explicit authority for extra streams.
# RU: Независимые от размещения byte channels FlashVM и capability дополнительных потоков.
include "sys.m";
include "vfsexport.m";
Terminal: module
{
    PATH: con "terminal.dis"; # Generic runtime helper, no application-specific client.


    # EN/RU: Ordinary managed FD owners and an explicit live channel root; no wire pointers.
    # Обычные managed FD owners и explicit live root; root не передаётся как native pointer.
    Session: adt
    {
        root: ref Vfsexport->Root; # Grant for additional named channels, retained independently of mounts.
        receive: ref Sys->FD; # Bytes from FlashVM to the executing module.
        send: ref Sys->FD; # Bytes from the module to FlashVM; may alias receive for duplex.
    };


    # EN/RU: Initialize immutable builtin interface before concurrent library use; raises on absence.
    # Инициализировать builtin до concurrent use; отсутствие вызывает exception.
    init: fn();


    # EN/RU: Open this process's inherited launch channels directly; owned session/nil, native wait rules.
    # Открыть inherited каналы своего launch без namespace lookup; owned session/nil, native wait rules.
    current: fn(): ref Session;


    # EN/RU: Open duplex same-name or receive/send pair on explicit grant; owned session/nil.
    # Открыть duplex либо pair на grant; owned session/nil, без mount/create/truncate/fallback.
    connect: fn(root: ref Vfsexport->Root, receive, send: string): ref Session;


    # EN/RU: Open additional live channel with Sys OREAD/OWRITE/ORDWR; owned FD/nil.
    # Открыть дополнительный live channel в session.root; права решает provider, FD/nil.
    open: fn(session: ref Session, name: string, mode: int): ref Sys->FD;
};
