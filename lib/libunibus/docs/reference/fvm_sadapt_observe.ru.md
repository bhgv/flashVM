# fvm_sadapt_observe

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sadapt_observe.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_observe(FVM_SAdapt *s, const FVM_SAdaptFeedback *f, FVM_SAdaptDecision *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `FVM_SAdapt *s` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `f` | `const FVM_SAdaptFeedback *f` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `out` | `FVM_SAdaptDecision *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sadapt_observe: выбранные host решения адаптации связи и гистерезис.

### Сохранённый контракт implementation/header

Consume one validated report atomically. Duplicate/old/wrong identity or pending proposal
CONFLICT; malformed counts INVALID. Unknown RTT forbids growth. Missing+late+damaged all
count as unusable by deadline. INSUFFICIENT resets streaks but does not spend cooldown.
Errors leave state/decision unchanged. A proposal never changes current geometry or sends
control traffic.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/sadapt-c/part-001.inc:222](../../protocol/readable_parts/sadapt-c/part-001.inc#L222)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
