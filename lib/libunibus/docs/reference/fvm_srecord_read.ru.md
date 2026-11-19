# fvm_srecord_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_srecord_read.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_srecord_read(const void *input, size_t size, size_t limit, FVM_SRecord *view, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Заимствованный вход; сохраняется всё время использования record view. |
| `size` | `size_t size` | Доступные входные байты; последующие записи могут остаться непоглощёнными. |
| `limit` | `size_t limit` | Допустимые body-байты, включая encoded tag: 1..FVM_SRECORD_BODY_MAX. |
| `view` | `FVM_SRecord *view` | Проверенная заимствованная запись, публикуемая только при OK. |
| `used` | `size_t *used` | Байты ровно одной записи при OK; иначе не меняется. |

## Описание

fvm_srecord_read: канонические целые, framing S-записей и проверка checksum.

### Сохранённый контракт implementation/header

limit is 1..BODY_MAX and counts tag+payload; pass BODY_MAX explicitly for the full profile.
MORE means retain/retry the prefix. Checksum is verified before tag interpretation.
OK publishes one record and consumed extent; trailing records remain unconsumed.
Caller must initialize neither view nor used; both remain unchanged on non-OK.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int invalid = fvm_srecord_read(record.payload, record.size, BRIDGE_LIMIT + 32, &request, &used) != FVM_S_OK;
```

[Complete calling context](../../examples/bridge/receive.c#L43)

## См. также

- [Implementation: protocol/srecord.c:389](../../protocol/srecord.c#L389)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
