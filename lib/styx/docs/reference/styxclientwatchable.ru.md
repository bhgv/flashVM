# styxclientwatchable

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwatchable.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientwatchable(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientwatchable: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Local capability snapshot;1 if live carrier owner has at least two RPC slots,0 otherwise.
Local capability snapshot;1 для live carrier owner минимум с двумя RPC slots.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientwatchable(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L33)

## См. также

- [Implementation: client/fd.c:193](../../client/fd.c#L193)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
