# Copyright (c) 2026 bhgv
# EN: Interface for six exports in one mounted Dis module.
# RU: Интерфейс шести экспортов одного смонтированного модуля Dis.
include "ref.m";

Loadedset: module {
    tally: fn(amount: int): int; # Accumulate one narrow value.
    wide: fn(value: big): big; # Read the same state through a wide scalar call.
    pair: fn(value: (int,big)): (int,big); # Accumulate through a tuple.
    revise: fn(value: ref Cell): (int,big); # Write back one object.
    blend: fn(seed: int, value: (int,big), delta: big): (int,(int,big)); # Read three arguments.
    exchange: fn(first, second: ref Cell): (int,big); # Preserve alias identity.
};
