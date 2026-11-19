# canqlock

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](canqlock.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int canqlock(QLock *q);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `q` | `QLock *q` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

canqlock: исходная POSIX-поддержка lib9 lock.

### Сохранённый контракт implementation/header

flag

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
canqlock(&l->k); /* block writers if we are the first reader */
```

[Complete calling context](../../vendor/inferno/lib9/lock.c#L188)

## См. также

- [Implementation: vendor/inferno/lib9/lock.c:126](../../vendor/inferno/lib9/lock.c#L126)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
