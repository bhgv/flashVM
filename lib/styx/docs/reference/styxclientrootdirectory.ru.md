# styxclientrootdirectory

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientrootdirectory.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientrootdirectory(StyxClient *client);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |

## Описание

styxclientrootdirectory: нативные client-операции fid и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Copy immutable attached root kind,1 directory/0 file/-1 NULL or auth root.
Caller retains client; no protocol request or mutable server metadata query.
Копировать immutable attach root kind:1 directory/0 file/-1 invalid; без protocol request.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientrootdirectory(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L14)

## См. также

- [Implementation: client/fid.c:48](../../client/fid.c#L48)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
