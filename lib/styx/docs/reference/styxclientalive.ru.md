# styxclientalive

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientalive.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientalive(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientalive: нативные client-операции session и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Snapshot terminal transport/framing state under connection lock;1/0.
Not a heartbeat or a permission check. Снимок terminal state; не heartbeat/права.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientalive(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L420)

## См. также

- [Implementation: client/readable_parts/session-c/part-001.inc:332](../../client/readable_parts/session-c/part-001.inc#L332)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
