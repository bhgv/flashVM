# styxphypending

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphypending.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

size_t styxphypending(Styxserver *server);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |

## Описание

styxphypending: реализация phy/lifetime и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner-only unsent encoded bytes,0 invalid/empty; never interprets queued contents.
Owner-only unsent bytes,0 invalid/empty, без интерпретации contents.

## Возвращаемое значение

`size_t`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: server/phy/lifetime.c:350](../../server/phy/lifetime.c#L350)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
