# fvm_sadapt_feedback_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sadapt_feedback_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_feedback_write(const FVM_SAdaptFeedback *f, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `const FVM_SAdaptFeedback *f` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sadapt_feedback_write: выбранные host решения адаптации связи и гистерезис.

### Сохранённый контракт implementation/header

Compact tag-zero report codec, no padding. Unknown subservice/command UNKNOWN; bad metadata
INVALID, checksum/prefix errors forwarded. Outputs atomic; read consumes one record only.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/sadapt-c/part-002.inc:20](../../protocol/readable_parts/sadapt-c/part-002.inc#L20)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
