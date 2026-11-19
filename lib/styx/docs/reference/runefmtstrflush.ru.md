# runefmtstrflush

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](runefmtstrflush.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

Rune *runefmtstrflush(Fmt *f);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |

## Описание

runefmtstrflush: исходная POSIX-поддержка lib9 fmtstr.

### Сохранённый контракт implementation/header

Terminate an aligned Rune view without transferring or freeing caller ownership.

## Возвращаемое значение

`Rune *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = runefmtstrflush(&f);
```

[Complete calling context](../../vendor/inferno/lib9/vsmprint.c#L299)

## См. также

- [Implementation: vendor/inferno/lib9/fmtstr.c:57](../../vendor/inferno/lib9/fmtstr.c#L57)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
