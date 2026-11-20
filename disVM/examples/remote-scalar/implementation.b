# Copyright (c) 2026 bhgv
# EN: Provider-local implementation with private resource and persistent state.
# RU: Provider-local реализация с приватным ресурсом и постоянным состоянием.
implement Compute;
include "privatevalue.m";
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
    dependency := load Privatevalue Privatevalue->PATH;
    if (dependency == nil) {
        raise "provider dependency absent";
    }
    return value + dependency->read() + calls;
}
