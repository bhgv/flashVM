# styxclientreadwatch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientreadwatch.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientreadwatch(
    StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `offset` | `unsigned long long offset` | Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл. |
| `bytes` | `void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |
| `result` | `StyxTransfer *result` | Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект. |

## Описание

styxclientreadwatch: нативные client-операции io и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Long-lived native read without ordinary request deadline; explicit token required.
Same precision/flush fences as readcancel. Heartbeat/terminal/cancel still operate.
Caller pins/serializes fid and token; requires watchable client, uses one RPC slot.
Долгий native read без обычного deadline; explicit token, heartbeat/cancel сохраняются.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
void styxclientreadwatch(StyxFid *, unsigned long long, void *, size_t, StyxCancel *, StyxTransfer *);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L62)

## См. также

- [Implementation: client/io.c:248](../../client/io.c#L248)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
