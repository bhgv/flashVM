# styxclientread

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientread.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientread(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result);
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
| `result` | `StyxTransfer *result` | Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект. |

## Описание

styxclientread: нативные client-операции io и время жизни.

### Сохранённый контракт implementation/header

EN/RU: One actual Tread/Twrite chunk, signed64 offset range; exact confirmed
prefix, no auto retry. Uncertain data effects poison further data calls on fid.
Один Tread/Twrite chunk; точный prefix, без retry; uncertain effects запрещают I/O.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxclientread(fid, offset, bytes, sizeof(bytes), &result);
```

[Complete calling context](../../examples/client.c#L67)

## См. также

- [Implementation: client/io.c:116](../../client/io.c#L116)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
