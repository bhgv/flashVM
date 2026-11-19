# styxclientretain

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientretain.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientretain(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientretain: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Retain borrowed live client;1/0 overflow. Drop with clientfree.
Удержать borrowed live client;1/0 overflow, release через clientfree.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientretain(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L339)

## См. также

- [Implementation: client/fd.c:54](../../client/fd.c#L54)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
