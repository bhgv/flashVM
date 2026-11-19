# fvm_sinteger_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sinteger_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_sinteger_write(const FVM_SInteger *value, unsigned char *out, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `value` | `const FVM_SInteger *value` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `out` | `unsigned char *out` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sinteger_write: канонические целые, framing S-записей и проверка checksum.

### Сохранённый контракт implementation/header

Canonical big-endian signed magnitude. Nonoverlapping outputs required. No allocation/VM.
Write/read value and used only on OK; null input is valid only when size is zero (MORE).
Write needs valid value/output/used. Read needs value/used. Maximum magnitude is 2^64-1.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fvm_sinteger_write(&n, out, FVM_SRECORD_INTEGER_MAX, &used);
```

[Complete calling context](../../protocol/readable_parts/sadapt-c/part-001.inc#L382)

## См. также

- [Implementation: protocol/srecord.c:53](../../protocol/srecord.c#L53)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
