# walk_leave

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](walk_leave.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/walk/walk.h>

int walk_leave(WalkScope *scope);
```

## Библиотека

Styx::walk; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `scope` | `WalkScope *scope` | Локальный guard активного обхода запроса; успешный enter требует соответствующего leave. |

## Описание

walk_leave: реализация walk/walk и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Pop exactly this active scope and release imported/deep storage;1/0 on unbalanced/wrong-thread use.
A failed enter must not be left. No callbacks, I/O or managed owners.
Pop active scope, освободить imported/deep storage;1/0 при unbalanced/wrong-thread use. Без callbacks/I/O.

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::walk; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: walk/walk.c:285](../../walk/walk.c#L285)
- [Public declaration](../../walk/walk.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
