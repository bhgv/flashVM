# styxclientfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientfree.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientfree(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientfree: нативные client-операции session и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Drop connection reference after callers retire; live fids pin transport.
Final release closes transport once; server retires root on disconnect. NULL allowed.
Поглотить ссылку; live fids удерживают transport; NULL допустим.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxclientfree(client);
```

[Complete calling context](../../examples/client.c#L82)

## См. также

- [Implementation: client/readable_parts/session-c/part-001.inc:358](../../client/readable_parts/session-c/part-001.inc#L358)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
