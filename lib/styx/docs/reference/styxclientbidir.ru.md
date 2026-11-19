# styxclientbidir

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientbidir.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientbidir(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientbidir: нативные client-операции phy и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Borrow live client;1 explicitly bidir-session carrier/0 ordinary or NULL.
This immutable local capability is neither reverse-server liveness nor access authority.
Borrow live client;1 bidir-session carrier/0 ordinary/NULL, не liveness/authority.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientbidir(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L220)

## См. также

- [Implementation: client/phy.c:59](../../client/phy.c#L59)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
