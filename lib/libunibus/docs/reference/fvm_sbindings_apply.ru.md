# fvm_sbindings_apply

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sbindings_apply.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_sbindings_apply(FVM_SBindings *table, const void *input, size_t size, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `table` | `FVM_SBindings *table` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sbindings_apply: привязки числовых тегов направления к идентичности схемы.

### Сохранённый контракт implementation/header

Decode/checksum one complete tag-zero record and atomically append all new associations.
Exact repeats are idempotent; rebinding a tag returns CONFLICT, duplicates in one record
INVALID. Unknown payload version is INVALID. Input/outputs/table/slots must not overlap.
Failed/incomplete records leave every slot/count/used unchanged. No VM execution occurs.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fvm_sbindings_apply(bindings, encoded, size, &used) == FVM_S_OK;
```

[Complete calling context](../../examples/bridge/main.c#L83)

## См. также

- [Implementation: protocol/sbindings.c:348](../../protocol/sbindings.c#L348)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
