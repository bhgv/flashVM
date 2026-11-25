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
        wire: array of byte; # Copy of the original issuer descriptor.
        writable: int; # Exact access requested by the original descriptor.
        payload: fn(cell: self ref Cell): array of byte;
        read: fn(cell: self ref Cell): int;
        write: fn(cell: self ref Cell, value: int);
    };

    # EN/RU: Initialize the Sys binding. / Подключить интерфейс Sys.
    init: fn();

    # EN: Validate one channel payload and open its exact exported cell on root.
    # RU: Проверить сообщение канала и открыть точную ячейку через root.
    accept: fn(root: string, bytes: array of byte): ref Cell;
};
