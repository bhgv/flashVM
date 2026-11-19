# fvm_sadapt_commit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sadapt_commit.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_commit(FVM_SAdapt *s, const FVM_SControlLimits *selected);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `FVM_SAdapt *s` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `selected` | `const FVM_SControlLimits *selected` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sadapt_commit: выбранные host решения адаптации связи и гистерезис.

### Сохранённый контракт implementation/header

Confirm the exact pending ladder geometry only after the external control ACK committed.
Clamped/different ACK CONFLICT; host rejects/rebuilds ladder for unsupported selections.
Successful commit starts holdoff; no payload/frame reinterpretation is performed here.

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

- [Implementation: protocol/readable_parts/sadapt-c/part-001.inc:314](../../protocol/readable_parts/sadapt-c/part-001.inc#L314)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
