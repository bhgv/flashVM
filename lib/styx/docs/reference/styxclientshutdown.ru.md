# styxclientshutdown

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientshutdown.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientshutdown(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientshutdown: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Explicit shutdown on native I/O worker, may wait for native owner retirement.
No guest execution/pause; final clientfree itself remains nonwaiting.
Явный native shutdown может ждать owner retirement; clientfree не ждёт.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxclientshutdown(client);
```

[Complete calling context](../../examples/client.c#L85)

## См. также

- [Implementation: client/fd.c:84](../../client/fd.c#L84)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
