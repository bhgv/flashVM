# styxclientwalk

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwalk.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxFid *styxclientwalk(StyxClient *client, const char *relative);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `relative` | `const char *relative` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientwalk: нативные client-операции walk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Walk canonical relative path, dot for root; owned independent fid/NULL.
Reject absolute/empty/dotdot/embedded dot components before wire I/O. Multiple
MAXWELEM batches preserve the same new fid. Calls serialized by connection mutex.
Пройти canonical relative path; свой fid/NULL, без выхода выше root.

## Возвращаемое значение

`StyxFid *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxFid *fid = styxclientwalk(client, argv[2]); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
```

[Complete calling context](../../examples/client.c#L60)

## См. также

- [Implementation: client/walk.c:226](../../client/walk.c#L226)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
