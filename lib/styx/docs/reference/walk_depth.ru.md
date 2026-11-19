# walk_depth

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](walk_depth.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/walk/walk.h>

size_t walk_depth(void);
```

## Библиотека

Styx::walk; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| — | void | Параметров нет. |

## Описание

walk_depth: реализация walk/walk и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Current native thread's active depth,0 between operations; no allocation or side effects.
Active depth текущего native thread;0 между operations, без allocation/side effects.

## Возвращаемое значение

`size_t`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::walk; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: walk/walk.c:319](../../walk/walk.c#L319)
- [Public declaration](../../walk/walk.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
