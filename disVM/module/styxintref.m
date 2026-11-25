# Copyright (c) 2026 bhgv
# EN: Receive one live StyxRPC variable as a typed Limbo ref.
# RU: Принимать живую переменную StyxRPC как типизированный Limbo ref.
Styxintref: module
{
    PATH: con "styxintref.dis"; # Installed module name.

    Cell: adt
    {
        fd: ref Sys->FD; # Held file on the issuing session.
        gate: chan of int; # Serializes operations on the retained fid cursor.
        root: string; # Issuer mount given on acceptance.
        wire: array of byte; # Copy of the original issuer descriptor.
        writable: int; # Exact access requested by the original descriptor.
        payload: fn(cell: self ref Cell): array of byte;
        read: fn(cell: self ref Cell): int;
        write: fn(cell: self ref Cell, value: int);
    };

    Channel: adt
    {
        root: string; # Mount of the peer issuing these references.
        wire: chan of array of byte; # Dedicated reference carrier.
        send: fn(port: self ref Channel, cell: ref Cell);
        recv: fn(port: self ref Channel): ref Cell;
    };

    # EN/RU: Bind an explicit byte carrier. / Привязать выделенный байтовый канал.
    channel: fn(root: string, wire: chan of array of byte): ref Channel;

    # EN/RU: Initialize the Sys binding. / Подключить интерфейс Sys.
    init: fn();

    # EN: Validate one channel payload and open its exact exported cell on root.
    # RU: Проверить сообщение канала и открыть точную ячейку через root.
    accept: fn(root: string, bytes: array of byte): ref Cell;
};
