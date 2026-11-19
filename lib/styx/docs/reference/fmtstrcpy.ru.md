# fmtstrcpy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fmtstrcpy.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int fmtstrcpy(Fmt *f, char *s);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |
| `s` | `char *s` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

fmtstrcpy: исходная POSIX-поддержка lib9 dofmt.

### Сохранённый контракт implementation/header

public helper routine: fmt out a null terminated string already in hand

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fmtstrcpy(fmt, buf);
```

[Complete calling context](../../vendor/inferno/lib9/fcallfmt.c#L190)

## См. также

- [Implementation: vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc:49](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L49)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
