# styxclientclunkcancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientclunkcancel.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientclunkcancel(StyxFid *fid, StyxCancel *cancel);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |

## Описание

styxclientclunkcancel: нативные client-операции clunk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume owned fid always; cancellable Tclunk on carrier-owner, async cleanup on legacy.
An unacknowledged request schedules one independent best-effort cleanup; never reuses fid numbers.
Caller serializes/pins fid before transfer. Token is borrowed single-use, NULL keeps deadline.
Поглотить fid всегда; carrier-owner Tclunk отменяем, legacy cleanup async; без Rclunk
один independent best-effort cleanup. Token borrowed single-use, NULL оставляет deadline.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
void styxclientclunkcancel(StyxFid *fid, StyxCancel *cancel);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L124)

## См. также

- [Implementation: client/clunk.c:54](../../client/clunk.c#L54)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
