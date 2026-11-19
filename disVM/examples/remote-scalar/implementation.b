# Copyright (c) 2026 bhgv
# EN: Provider-local implementation with private resource and persistent state.
# RU: Provider-local реализация с приватным ресурсом и постоянным состоянием.
implement Compute;
include "sys.m";
include "compute.m";
calls: int; # Owned by the original serving Prog / Состояние original serving Prog.


# EN: Use provider-local file and count even failed calls; raise for value=-1.
# Return input+secret+call count. No remote pointer or client resource is used.
# RU: Читать provider-local файл и считать даже failed calls; value=-1 даёт raise.
# Вернуть input+secret+call count. Без remote pointer и клиентского ресурса.
compute(value: int): int
{
    calls++;
    if (value == -1) {
        raise "provider implementation failure";
    }
    sys := load Sys Sys->PATH;
    file := sys->open("provider-secret", Sys->OREAD);
    if (file == nil) { raise "provider resource absent"; }
    bytes := array[16] of byte;
    n := sys->read(file, bytes, len bytes);
    if (n <= 0) { raise "provider resource unreadable"; }
    return value + int string bytes[:n] + calls;
}
