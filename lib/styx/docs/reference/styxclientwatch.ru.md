# styxclientwatch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwatch.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *));
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `notify` | `StyxTerminal notify` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `state` | `void *state` | Непрозрачный контекст callback; время жизни/передача заданы создающим API. |
| `void (*release)(void *)` | `void (*release)(void *)` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientwatch: нативные client-операции session и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Adopt state only on success, owned watch/NULL; terminal state replays once.
At most256 live watcher nodes/client. Retain client while registering. State
outlives already admitted callbacks; release runs after final unsubscribe/flight.
Принять state только при успехе; свой watch/NULL, sticky terminal replay.

## Возвращаемое значение

`StyxWatch *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *));
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L115)

## См. также

- [Implementation: client/readable_parts/session-c/part-002.inc:173](../../client/readable_parts/session-c/part-002.inc#L173)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
