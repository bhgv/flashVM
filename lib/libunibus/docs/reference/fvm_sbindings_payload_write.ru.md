# fvm_sbindings_payload_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sbindings_payload_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sbindings.h>

FVM_SResult
fvm_sbindings_payload_write(const FVM_SBinding *bindings, size_t count, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `bindings` | `const FVM_SBinding *bindings` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `count` | `size_t count` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sbindings_payload_write: привязки числовых тегов направления к идентичности схемы.

### Сохранённый контракт implementation/header

Encode only the tag-zero payload, then wrap it with fvm_srecord_write(CONTROL_TAG,...).
Payload = schema version, count, repeated(tag, schema version, identity length, identity
bytes). Validates the full list/capacity before writing; no duplicate/zero tags, no NUL/space
identities. Input associations/output must not overlap; used remains unchanged on failure.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_sbindings_payload_write(&assignment, 1, metadata, sizeof(metadata), &metadata_size);
```

[Complete calling context](../../examples/nested.c#L73)

## См. также

- [Implementation: protocol/sbindings.c:285](../../protocol/sbindings.c#L285)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
