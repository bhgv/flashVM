# Copyright (c) 2026 bhgv
# EN: Portable dependency interface, same signature for every granted code source.
# RU: Переносимый интерфейс зависимости, одинаковая сигнатура для всех выданных источников кода.
Value: module
{
    value: fn(): int; # Owner-local module call, no transport/native pointer argument.
};
