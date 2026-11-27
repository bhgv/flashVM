# Copyright (c) 2026 bhgv
# EN: Typed Limbo handle to an Int32 export of either VM in the pair.
# RU: Типизированный дескриптор Limbo для экспорта Int32 любой VM в паре.

Styxmodref: module
{
    PATH: con "styxmodref.dis"; # Installed Limbo adapter name.

    Cell: adt
    {
        fd: ref Sys->FD; # Private binding to the exact issuer through the SDK.

        # EN: Read the original signed Int32 through cell.
        # RU: Прочитать исходный знаковый Int32 через cell.
        read: fn(cell: self ref Cell): int;

        # EN: Replace the original signed Int32 if the captured mode allows it.
        # RU: Заменить исходный знаковый Int32, если позволяет захваченный режим.
        write: fn(cell: self ref Cell, value: int);

        # EN: Copy the complete named-channel frame for forwarding on this pair.
        # RU: Скопировать полный кадр именованного канала для пересылки в этой паре.
        payload: fn(cell: self ref Cell): array of byte;

        # EN: Release this handle. A second close is harmless.
        # RU: Освободить дескриптор. Повторное закрытие безопасно.
        close: fn(cell: self ref Cell);
    };

    # EN: Load Sys before using the adapter.
    # RU: Загрузить Sys перед использованием адаптера.
    init: fn();

    # EN: Pin mounted/<image>/.var/<member> on the attached peer. writable is 0 or 1.
    # RU: Удержать mounted/<образ>/.var/<имя> на подключённой стороне. writable равен 0 или 1.
    capture: fn(path: string, writable: int): ref Cell;

    # EN: Pin the local publication at a root-relative export path. writable is 0 or 1.
    # RU: Удержать местную публикацию по пути от корня пары. writable равен 0 или 1.
    capturelocal: fn(path: string, writable: int): ref Cell;

    # EN: Accept a complete named-channel frame for either owner. Open on first value I/O.
    # RU: Принять полный кадр именованного канала от любого владельца. Открыть при первом I/O значения.
    adopt: fn(frame: array of byte): ref Cell;
};
