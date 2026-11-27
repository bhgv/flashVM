# styxclientwalkfromcancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwalkfromcancel.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxFid *styxclientwalkfromcancel(StyxFid *base, const char *relative, StyxCancel *cancel, StyxTransfer *result);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `base` | `StyxFid *base` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `relative` | `const char *relative` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |
| `result` | `StyxTransfer *result` | Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект. |

## Описание

styxclientwalkfromcancel: нативные client-операции walk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Cancellable logical walk for native PHY clients; owned fid/NULL, required result.
One outer token covers all MAXWELEM fragments and cannot be reused. Result uses
StyxTransfer with count0; success means the complete walk was acknowledged.
Failure cleanup is native detached best-effort clunk, never a caller RPC wait.
Base overload borrows a pinned unopened fid; same confinement as walkfrom.
Отменяемый logical walk; свой fid/NULL, required result, count0; token один на все
fragments. Cleanup без RPC wait caller; base удержан и unopened на время вызова.

## Возвращаемое значение

`StyxFid *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxFid *styxclientwalkfromcancel(StyxFid *base, const char *relative, StyxCancel *cancel, StyxTransfer *result);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L239)

## См. также

- [Implementation: client/walk.c:310](../../client/walk.c#L310)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
