# Copyright (c) 2026 bhgv
# EN: Provider application explicitly loads its normal typed dependency from the instance VFS.
# RU: Провайдерское приложение загружает обычную typed-зависимость из VFS своего экземпляра.
implement Value;
include "value.m";


# EN/RU: Load a dependency on the same provider runtime; return -1 on ordinary load refusal.
# Загрузить зависимость в том же provider runtime; вернуть -1 при обычном отказе load.
value(): int
{
    dep := load Value "/dep.dis";
    if (dep == nil) {
        return -1;
    }
    return dep->value();
}
