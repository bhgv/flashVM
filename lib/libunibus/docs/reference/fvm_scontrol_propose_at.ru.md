# fvm_scontrol_propose_at

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_scontrol_propose_at.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_propose_at(FVM_SControlExchange *x,

                                    unsigned long id,
                                    const FVM_SControlLimits *limits,
                                    const FVM_SControlBoundary *boundary,
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
| `boundary` | `const FVM_SControlBoundary *boundary` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `out` | `FVM_SControlMessage *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_scontrol_propose_at: коррелированное согласование размера/профиля отдельного тега.

### Сохранённый контракт implementation/header

Boundary-aware offer: nonzero stream/epoch/first object uses OFFER_AT/ACCEPT_AT. The ACK
must echo the complete boundary exactly, not silently move it. All-zero boundary selects
the generic offer behavior. Host chooses a future object and drains/retires old receiver
storage without pausing the overall timeline. Sizes may increase or decrease within ceiling.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_scontrol_propose_at(x, id, limits, &none, out);
```

[Complete calling context](../../protocol/readable_parts/scontrol-c/part-002.inc#L201)

## См. также

- [Implementation: protocol/readable_parts/scontrol-c/part-001.inc:433](../../protocol/readable_parts/scontrol-c/part-001.inc#L433)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
