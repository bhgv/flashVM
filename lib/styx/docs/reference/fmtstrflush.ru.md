# fmtstrflush

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fmtstrflush.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *fmtstrflush(Fmt *f);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |

## Описание

fmtstrflush: исходная POSIX-поддержка lib9 fmtstr.

### Сохранённый контракт implementation/header

Terminate only a valid byte view, retaining caller ownership when validation fails.

## Возвращаемое значение

`char *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fmtstrflush(&f);
```

[Complete calling context](../../vendor/inferno/lib9/vsmprint.c#L176)

## См. также

- [Implementation: vendor/inferno/lib9/fmtstr.c:33](../../vendor/inferno/lib9/fmtstr.c#L33)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
