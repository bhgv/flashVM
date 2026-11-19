# styxclientstat

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientstat.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientstat(StyxFid *fid, StyxMeta *output);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `output` | `StyxMeta *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |

## Описание

styxclientstat: нативные client-операции stat и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Query current remote fid metadata;1/0, output changes only on success.
Tstat не открывает путь заново;1/0, output только при успехе.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
good = good && styxclientstat(fid, &metadata) && !strcmp(metadata.name, "status") && metadata.length == 21;
```

[Complete calling context](../../examples/memory_phy/main.c#L32)

## См. также

- [Implementation: client/stat.c:130](../../client/stat.c#L130)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
