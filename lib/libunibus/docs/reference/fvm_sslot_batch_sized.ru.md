# fvm_sslot_batch_sized

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sslot_batch_sized.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sslot.h>

FVM_SResult fvm_sslot_batch_sized(
    const void *input, size_t size, size_t slot_bytes, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `slot_bytes` | `size_t slot_bytes` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `notify` | `FVM_SSlotNotify notify` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `userdata` | `void *userdata` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `report` | `FVM_SSlotReport *report` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sslot_batch_sized: проверяемая геометрия fixed slots и размеры compact records.

### Сохранённый контракт implementation/header

Corrupt slots are reported independently; even damaged lengths cannot redirect iteration.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_sslot_batch_sized(input, size, FVM_SSLOT_BYTES, notify, userdata, report);
```

[Complete calling context](../../protocol/sslot.c#L252)

## См. также

- [Implementation: protocol/sslot.c:189](../../protocol/sslot.c#L189)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
