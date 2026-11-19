# fvm_sslot_read_sized

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sslot_read_sized.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sslot.h>

FVM_SResult fvm_sslot_read_sized(const void *input, size_t size, size_t slot_bytes, FVM_SRecord *view, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `slot_bytes` | `size_t slot_bytes` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `view` | `FVM_SRecord *view` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sslot_read_sized: проверяемая геометрия fixed slots и размеры compact records.

### Сохранённый контракт implementation/header

Never let corrupted embedded framing cross the externally fixed boundary.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_sslot_read_sized(input, size, profile->slot_bytes, &record, &used);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-001.inc#L261)

## См. также

- [Implementation: protocol/sslot.c:107](../../protocol/sslot.c#L107)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
