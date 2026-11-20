# Copyright (c) 2026 bhgv
# EN: Owner-local dependency providing the sample's private value.
# RU: Локальная зависимость владельца, которая выдаёт значение для примера.
Privatevalue: module
{
    PATH: con "privatevalue.dis"; # Resolved in the implementation's namespace.
    read: fn(): int;
};
