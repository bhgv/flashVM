# fvm_sadapt_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sadapt_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_init(FVM_SAdapt *out, const FVM_SAdaptConfig *c);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `FVM_SAdapt *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `c` | `const FVM_SAdaptConfig *c` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sadapt_init: выбранные host решения адаптации связи и гистерезис.

### Сохранённый контракт implementation/header

Validate/copy complete ladder and policy; errors preserve state and borrow nothing. State
must remain at a stable address after init (config.profiles references its internal table).

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/sadapt-c/part-001.inc:170](../../protocol/readable_parts/sadapt-c/part-001.inc#L170)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
