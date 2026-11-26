# Copyright (c) 2026 bhgv
# EN: Blocking named StyxRPC channels exposed by the paired DisVM namespace.
# RU: Блокирующие именованные каналы StyxRPC в пространстве парной DisVM.
Styxnamed: module
{
    PATH: con "styxnamed.dis"; # Compiled portable Limbo implementation.
    Snapshot, Intref, Number, Text, Boolean, Null, Undefined: con iota+1; # Wire type byte.

    Port: adt
    {
        fd: ref Sys->FD; # One participant; GC of the fd also releases the name.

        # EN: Wait for a write and return its tagged, counted frame.
        # RU: Дождаться записи и вернуть кадр с тегом и длиной.
        recv: fn(port: self ref Port): array of byte;

        # EN: Wait for a participant to accept this nonempty frame.
        # RU: Дождаться принятия непустого кадра участником.
        send: fn(port: self ref Port, frame: array of byte);

        # EN: Drop this participant. Repeated close is harmless.
        # RU: Освободить участника. Повторное закрытие безопасно.
        close: fn(port: self ref Port);
    };

    # EN: Load Sys before opening channels.
    # RU: Загрузить Sys перед открытием каналов.
    init: fn();

    # EN: Open or create at a canonical absolute path on the current paired namespace.
    # RU: Открыть или создать канал по абсолютному пути текущего парного пространства.
    open: fn(path: string): ref Port;
};
