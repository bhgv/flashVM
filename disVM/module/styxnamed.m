# Copyright (c) 2026 bhgv
# EN: Blocking named StyxRPC channels exposed by the paired DisVM namespace.
# RU: Блокирующие именованные каналы StyxRPC в пространстве парной DisVM.
Styxnamed: module
{
    PATH: con "styxnamed.dis"; # Compiled portable Limbo implementation.
    Snapshot, Intref, Number, Text, Boolean, Null, Undefined, Cellref: con iota+1; # Wire type byte.

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

    Cell: adt
    {
        fd: ref Sys->FD; # Owned lease preserving the cell independently of the channel.
        frame: array of byte; # Private descriptor copy for forwarding.

        # EN: Read the current tagged value frame.
        # RU: Прочитать текущий кадр значения с тегом.
        get: fn(cell: self ref Cell): array of byte;

        # EN: Replace the value with a complete tagged frame.
        # RU: Заменить значение полным кадром с тегом.
        set: fn(cell: self ref Cell, value: array of byte);

        # EN: Copy the descriptor for sending this same cell again.
        # RU: Скопировать дескриптор для повторной передачи той же ячейки.
        payload: fn(cell: self ref Cell): array of byte;

        # EN: Release this lease while other leases remain valid.
        # RU: Освободить эту аренду, сохраняя действительность остальных.
        close: fn(cell: self ref Cell);
    };

    # EN: Attach a frame received from a channel before its next operation.
    # RU: Принять полученный кадр ссылки до следующей операции на канале.
    adopt: fn(frame: array of byte): ref Cell;

    # EN: Create one shared value cell. The returned Cell is its first lease.
    # RU: Создать общую ячейку значения. Возвращаемый Cell — её первая аренда.
    share: fn(value: array of byte): ref Cell;

    # EN: Load Sys before opening channels.
    # RU: Загрузить Sys перед открытием каналов.
    init: fn();

    # EN: Open or create at a canonical absolute path on the current paired namespace.
    # RU: Открыть или создать канал по абсолютному пути текущего парного пространства.
    open: fn(path: string): ref Port;
};
