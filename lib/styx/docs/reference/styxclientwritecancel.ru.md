# styxclientwritecancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwritecancel.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientwritecancel(
    StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `offset` | `unsigned long long offset` | Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл. |
| `bytes` | `const void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |
| `result` | `StyxTransfer *result` | Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект. |

## Описание

styxclientwritecancel: нативные client-операции io и время жизни.

### Сохранённый контракт implementation/header

EN/RU: See client.h; preserve original write acknowledgement if it wins the flush race.
См. client.h; сохранить original write acknowledgement при победе над flush.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: client/io.c:210](../../client/io.c#L210)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
