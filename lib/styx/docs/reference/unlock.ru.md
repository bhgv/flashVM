# unlock

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](unlock.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void unlock(Lock *l);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `l` | `Lock *l` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

unlock: исходная POSIX-поддержка lib9 lock.

### Сохранённый контракт implementation/header

synchronization

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
unlock(&q->use);
```

[Complete calling context](../../vendor/inferno/lib9/lock.c#L98)

## См. также

- [Implementation: vendor/inferno/lib9/lock.c:75](../../vendor/inferno/lib9/lock.c#L75)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
