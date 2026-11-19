# dofmt

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](dofmt.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int dofmt(Fmt *f, char *fmt);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

dofmt: исходная POSIX-поддержка lib9 dofmt.

### Сохранённый контракт implementation/header

format the output into f->to and return the number of characters fmted

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
n = dofmt(f, fmt);
```

[Complete calling context](../../vendor/inferno/lib9/fmtprint.c#L53)

## См. также

- [Implementation: vendor/inferno/lib9/readable_parts/dofmt-c/part-001.inc:39](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-001.inc#L39)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
