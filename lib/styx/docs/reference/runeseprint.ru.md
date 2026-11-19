# runeseprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](runeseprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

Rune *runeseprint(Rune *buf, Rune *e, char *fmt, ...);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `Rune *buf` | Unicode rune либо буфер rune, отличный от числа UTF-8 байт. |
| `e` | `Rune *e` | Unicode rune либо буфер rune, отличный от числа UTF-8 байт. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `...` | `...` | Переменные аргументы, соответствующие переданному format. |

## Описание

runeseprint: исходная POSIX-поддержка lib9 runeseprint.

## Возвращаемое значение

`Rune *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/runeseprint.c:23](../../vendor/inferno/lib9/runeseprint.c#L23)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
