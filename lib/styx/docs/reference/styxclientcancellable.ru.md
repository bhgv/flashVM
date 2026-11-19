# styxclientcancellable

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientcancellable.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientcancellable(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientcancellable: нативные client-операции fid и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Immutable explicit-cancel capability,1 for carrier-owner,0 for NULL/legacy; no I/O.
Immutable explicit-cancel capability:1 carrier-owner,0 NULL/legacy; без I/O/прав.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientcancellable(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L16)

## См. также

- [Implementation: client/fid.c:67](../../client/fid.c#L67)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
