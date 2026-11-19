# isalpharune

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](isalpharune.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int isalpharune(Rune c);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `c` | `Rune c` | Unicode rune либо буфер rune, отличный от числа UTF-8 байт. |

## Описание

isalpharune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/readable_parts/runetypebody-6-2-0-h/part-001.inc:154](../../vendor/inferno/lib9/readable_parts/runetypebody-6-2-0-h/part-001.inc#L154)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
