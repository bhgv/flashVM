# runlock

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](runlock.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void runlock(RWlock *l);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `l` | `RWlock *l` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

runlock: исходная POSIX-поддержка lib9 lock.

### Сохранённый контракт implementation/header

Count of readers in lock

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/lock.c:230](../../vendor/inferno/lib9/lock.c#L230)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
