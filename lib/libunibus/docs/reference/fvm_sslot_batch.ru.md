# fvm_sslot_batch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sslot_batch.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sslot.h>

FVM_SResult
fvm_sslot_batch(const void *input, size_t size, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `notify` | `FVM_SSlotNotify notify` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `userdata` | `void *userdata` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `report` | `FVM_SSlotReport *report` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sslot_batch: проверяемая геометрия fixed slots и размеры compact records.

### Сохранённый контракт implementation/header

Accept zero through BATCH complete slots; partial trailing slot is INVALID before callbacks.
Every structurally bounded batch returns OK even if individual slots are damaged. Inspect
report/callback results. Invalid arguments leave report untouched and invoke no callbacks.

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

- [Implementation: protocol/sslot.c:250](../../protocol/sslot.c#L250)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
