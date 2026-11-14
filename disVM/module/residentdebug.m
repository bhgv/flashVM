# Copyright (c) 2026 bhgv
# EN: Bounded owner-location reader; owns copied display strings, never target pointers.
# RU: Чтение owner location с лимитами; владеет копиями строк, не target pointers.
Residentdebug: module
{
    PATH: con "residentdebug.dis"; # Relative library path selected by the application.


    # EN/RU: Immutable ticket capture copied into caller-owned managed values.
    # Неизменяемая ticket capture в managed values своего caller.
    Location: adt
    {
        sequence: big; # Owner-local boundary sequence, not a Module ID.
        nextpc: big; # Next original Dis instruction index; may equal instructions.
        instructions: int; # Captured module extent.
        flags: int; # Processctl LocationValid/truncation bits.
        name: string; # Copied module display name.
        path: string; # Copied UTF-8 path or complete truncated prefix.
    };


    # EN: Request and read a location with 1..1000 polling attempts; attempt to forget
    # its ticket. Return status/copy, nil on status failure/timeout; managed errors rethrow.
    # RU: Запросить location с 1..1000 poll attempts; пытаться забыть свой ticket на каждом выходе.
    # Вернуть status/копию, nil при отказе/timeout; managed errors передаются выше.
    inspect: fn(id: big, attempts: int): (int, ref Location);
};
