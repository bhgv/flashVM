# Copyright (c) 2026 bhgv
# EN: Native guest loader fixture with immutable module identity.
# RU: Fixture гостевого loader с immutable module identity.
implement Value;
include "value.m";


# EN/RU: Return this compiled module identity; no arguments or mutable state.
# Вернуть identity compiled module без аргументов и mutable state.
value(): int
{
    return 73;
}
