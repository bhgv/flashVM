# Copyright (c) 2026 bhgv
# EN: Local candidate for the owner library union.
# RU: Локальный вариант библиотеки в union владельца.
implement Wide;
include "wide.m";


# EN: Return a distinguishable value for the local candidate.
# RU: Вернуть отличимое значение для локального варианта.
echo(value: big): big
{
    return value + big 10;
}
