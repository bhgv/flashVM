# styxclientopencancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientopencancel.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `mode` | `int mode` | Режим операции/доступа; использовать протокольные константы, не суффикс имени. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |
| `result` | `StyxTransfer *result` | Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект. |

## Описание

styxclientopencancel: нативные client-операции open и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Cancellable Topen/Tstat require mountfd and result; return1/0, count0.
Original replies win over cancellation. Sent unacknowledged Topen poisons the fid:
no later open/data replay; caller still owns it and may stat/clunk. Tstat leaves
output untouched on every failure and does not poison the fid. Serialize/pin fid.
NULL cancel keeps the normal deadline; non-NULL tokens remain single-use.
Отменяемые Topen/Tstat:1/0, count0, result обязателен; original reply приоритетен.
Topen без acknowledgement блокирует повторный open/data; Tstat не меняет output
при отказе и не poison fid. Caller удерживает/сериализует fid; token single-use.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L309)

## См. также

- [Implementation: client/open.c:105](../../client/open.c#L105)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
