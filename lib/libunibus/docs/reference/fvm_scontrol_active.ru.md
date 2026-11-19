# fvm_scontrol_active

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_scontrol_active.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/scontrol.h>

FVM_SResult
fvm_scontrol_active(const FVM_SControlExchange *x, FVM_SControlLimits *limits, FVM_SControlBoundary *boundary);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `x` | `const FVM_SControlExchange *x` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `limits` | `FVM_SControlLimits *limits` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `boundary` | `FVM_SControlBoundary *boundary` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_scontrol_active: коррелированное согласование размера/профиля отдельного тега.

### Сохранённый контракт implementation/header

Copy the committed limits/boundary without exposing mutable exchange internals to adapters.
UNKNOWN before any agreement; INVALID for bad native state/outputs. Outputs remain unchanged
on error and must be disjoint from each other and the exchange. Owner-serialized, no I/O.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:222](../../protocol/readable_parts/scontrol-c/part-002.inc#L222)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
