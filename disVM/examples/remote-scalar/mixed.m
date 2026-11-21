# Copyright (c) 2026 bhgv
# EN: Typed imports for the mixed-width mounted library example.
# RU: Типизированные импорты смонтированной библиотеки со смешанными типами.
Mixed: module {
    compute: fn(value: int): int; # Scoped int / int с контекстом.
    wide: fn(value: big): big; # Scoped big / big с контекстом.
    raw: fn(value: big): big; # Unscoped big / big без контекста.
};
