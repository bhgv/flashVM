# fvm_srecord_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_srecord_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_srecord_write(
    unsigned long tag, const void *payload, size_t size, unsigned char *out, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `tag` | `unsigned long tag` | Беззнаковый тег направления 0..0xffffffff; transport tag 0 принадлежит обслуживанию. |
| `payload` | `const void *payload` | Заимствованные непрозрачные байты, возможно вложенные записи; отдельно от output/state. |
| `size` | `size_t size` | Байты payload; tag+payload должны помещаться в предел body-профиля. |
| `out` | `unsigned char *out` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Доступные выходные байты; нехватка места даёт LIMIT. |
| `used` | `size_t *used` | Полная кодированная длина при OK; при отказе не меняется. |

## Описание

fvm_srecord_write: канонические целые, framing S-записей и проверка checksum.

### Сохранённый контракт implementation/header

Record = unsigned checksum, unsigned length, unsigned tag, payload. Length counts
tag+payload. CRC covers exact length/tag/payload bytes; profile 1 retains
6+7*(length-varint-size-1) bits, capped at 32. Integers are canonical, so checksum may use
fewer bytes if its value is small. Write validates all sizes before output. Payload/output
must not overlap; no VM/GC/SDL. Outputs and used remain unchanged on failure. Tag <= U32,
body <= BODY_MAX.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_srecord_write(tag, bytes, size, (unsigned char *)output + header, capacity - header, &frame);
```

[Complete calling context](../../protocol/order/packet.c#L60)

## См. также

- [Implementation: protocol/srecord.c:362](../../protocol/srecord.c#L362)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
