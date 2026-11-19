# styxclientopen

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientopen.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientopen(StyxFid *fid, int mode);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `mode` | `int mode` | Режим операции/доступа; использовать протокольные константы, не суффикс имени. |

## Описание

styxclientopen: нативные client-операции open и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Open unopened fid in mode0/1/2 without create/truncate;1/0.
Caller serializes a fid's calls. Ropen iounit limits every later chunk.
An unacknowledged sent open fences later open/data calls; stat/clunk remain allowed.
Открыть fid без create/truncate;1/0, iounit ограничивает chunk. Sent open без
acknowledgement запрещает повторный open/data; stat/clunk остаются доступны.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int valid = fid && styxclientopen(fid, 0);
```

[Complete calling context](../../examples/client.c#L57)

## См. также

- [Implementation: client/open.c:79](../../client/open.c#L79)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
