# walk_import

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](walk_import.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/walk/walk.h>

int walk_import(WalkScope *scope, const void *input, size_t size);
```

## Библиотека

Styx::walk; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `scope` | `WalkScope *scope` | Локальный guard активного обхода запроса; успешный enter требует соответствующего leave. |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

walk_import: реализация walk/wire и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Append a validated copied remote history to this native thread; WALK_*; owned scope only on OK.
Borrow bytes through return only. Reject cycles/malformed/bounds before any provider callback, unwind on failure.
Добавить validated copied remote history; WALK_*, scope лишь при OK; bytes borrowed до return.
Cycles/malformed/bounds отказать до provider callback; rollback при failure.

## Возвращаемое значение

`int`; прямые константы результата тела: WALK_INVALID, WALK_LIMIT, WALK_NOMEM, WALK_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::walk; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: walk/wire.c:174](../../walk/wire.c#L174)
- [Public declaration](../../walk/walk.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
