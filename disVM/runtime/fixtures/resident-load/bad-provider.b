# Copyright (c) 2026 bhgv
# EN: Valid bytecode whose typed export must be rejected by the Burst importer.
# RU: Корректный байткод с экспортом, отвергаемым импортёром Burst по типу.
implement BadBurst;
include "bad-provider.m";


# EN: Return borrowed value's length; the incompatible importer must never call it.
# RU: Вернуть длину заимствованного value; несовместимый импортёр не должен его вызвать.
run(value: string): int
{
    return len value;
}
