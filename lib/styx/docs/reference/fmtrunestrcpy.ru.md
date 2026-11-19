# fmtrunestrcpy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fmtrunestrcpy.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int fmtrunestrcpy(Fmt *f, Rune *s);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |
| `s` | `Rune *s` | Unicode rune либо буфер rune, отличный от числа UTF-8 байт. |

## Описание

fmtrunestrcpy: исходная POSIX-поддержка lib9 dofmt.

### Сохранённый контракт implementation/header

public helper routine: fmt out a null terminated rune string already in hand

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fmtrunestrcpy(f, s);
```

[Complete calling context](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L169)

## См. также

- [Implementation: vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc:115](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L115)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
