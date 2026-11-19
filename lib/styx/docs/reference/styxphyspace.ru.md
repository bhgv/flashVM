# styxphyspace

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphyspace.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

unsigned styxphyspace(Styxserver *server);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |

## Описание

styxphyspace: реализация phy/lifetime и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner-only current free reply slots;0 invalid/failed/full, no I/O or permission decision.
Owner-only число свободных reply slots;0 invalid/failed/full, без I/O/permission decision.

## Возвращаемое значение

`unsigned`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: server/phy/lifetime.c:324](../../server/phy/lifetime.c#L324)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
