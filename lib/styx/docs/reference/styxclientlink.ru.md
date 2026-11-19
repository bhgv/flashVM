# styxclientlink

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientlink.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientlink(StyxClient *client, StyxLinkEvent *output);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `output` | `StyxLinkEvent *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |

## Описание

styxclientlink: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Copy local latest observation without I/O;1/0, output untouched on invalid input.
Копировать latest local observation без I/O;1/0, invalid не меняет output.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientlink(StyxClient *client, StyxLinkEvent *output);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L82)

## См. также

- [Implementation: client/fd.c:174](../../client/fd.c#L174)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
