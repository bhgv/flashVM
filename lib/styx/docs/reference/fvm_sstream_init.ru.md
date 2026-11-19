# fvm_sstream_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sstream_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_init(FVM_SStream *stream, void *buffer, size_t capacity, size_t limit);
```

## Библиотека

Styx::wire; C89

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `stream` | `FVM_SStream *stream` | Изменяемое состояние одного направления приёма; init до push. |
| `buffer` | `void *buffer` | Память приёма вызывающего, отдельно от stream и входных фрагментов; сохраняется на время работы decoder. |
| `capacity` | `size_t capacity` | Доступные байты buffer; могут ограничивать запись сильнее limit. |
| `limit` | `size_t limit` | Максимум байтов body, включая encoded tag: 1..FVM_SRECORD_BODY_MAX. |

## Описание

fvm_sstream_init: инкрементальный проверяемый ввод S-записей и время жизни буфера.

### Сохранённый контракт implementation/header

Initialize without touching buffer; capacity may be below limit and then enforces a tighter
bound. No allocation, locks, VM/GC or I/O. Output/storage must not overlap.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::wire; C89

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/sstream.c:84](../../protocol/sstream.c#L84)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
