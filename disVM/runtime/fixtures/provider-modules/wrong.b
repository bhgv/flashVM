# Copyright (c) 2026 bhgv
# EN: Real incompatible export; this module is parsed for metadata and never executed.
# RU: Настоящий incompatible export; module разбирается для metadata и не исполняется.
implement Wrong;
include "wrong.m";


# EN/RU: Return a big value; no input, effects or ownership transfer. It must not match Value.value(): int.
# Вернуть big value; без input/effects/ownership transfer. Не совместим с Value.value(): int.
value(): big
{
    return big 111;
}
