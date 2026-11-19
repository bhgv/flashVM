# fvm_scontrol_propose

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_scontrol_propose.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_propose(FVM_SControlExchange *x,
                                 unsigned long id,
                                 const FVM_SControlLimits *limits,
                                 FVM_SControlMessage *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `x` | `FVM_SControlExchange *x` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `id` | `unsigned long id` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `limits` | `const FVM_SControlLimits *limits` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `out` | `FVM_SControlMessage *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_scontrol_propose: коррелированное согласование размера/профиля отдельного тега.

### Сохранённый контракт implementation/header

Start one strictly increasing ID; reject overlapping negotiations CONFLICT and excess local
limits LIMIT. Output is an OFFER to encode/send, never immediate activation.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:195](../../protocol/readable_parts/scontrol-c/part-002.inc#L195)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
