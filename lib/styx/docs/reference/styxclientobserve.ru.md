# styxclientobserve

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientobserve.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *));
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `receive` | `StyxLinkReceive receive` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `state` | `void *state` | Непрозрачный контекст callback; время жизни/передача заданы создающим API. |
| `void (*release)(void *)` | `void (*release)(void *)` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientobserve: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Own state only on success; persistent watch/NULL, immediate current snapshot.
Caller pins client; ordered generations fence replay racing with native delivery.
Detach with clientunwatch; release follows all admitted callbacks, no guest calls.
Persistent watch/NULL принимает state при успехе; snapshot немедленно, generations ordered.

## Возвращаемое значение

`StyxWatch *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *));
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L63)

## См. также

- [Implementation: client/fd.c:153](../../client/fd.c#L153)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
