# _assert

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](_assert.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void _assert(char *message);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `message` | `char *message` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

_assert: реализация posix/fatal и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Report an upstream invariant and terminate; no allocation or formatter recursion.
Сообщить upstream invariant и завершить; без allocation/formatter recursion.

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

- [Implementation: ports/posix/fatal.c:22](../../ports/posix/fatal.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
