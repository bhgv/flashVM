# styxclientfiddirectory

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientfiddirectory.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientfiddirectory(StyxFid *fid);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |

## Описание

styxclientfiddirectory: нативные client-операции fid и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Borrow pinned fid; directory1/file0/invalid-1, no RPC.
Заимствовать pinned fid; directory1/file0/invalid-1, без RPC.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientfiddirectory(StyxFid *fid);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L159)

## См. также

- [Implementation: client/fid.c:86](../../client/fid.c#L86)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
