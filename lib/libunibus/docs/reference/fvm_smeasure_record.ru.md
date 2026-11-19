# fvm_smeasure_record

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smeasure_record.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_record(FVM_SMeasure *s, unsigned long part, unsigned int valid, unsigned long elapsed_ms);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `FVM_SMeasure *s` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `part` | `unsigned long part` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `valid` | `unsigned int valid` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

fvm_smeasure_record: измерения потока и явные входы адаптации.

### Сохранённый контракт implementation/header

Record one trusted zero-based part identity and host-relative monotonic elapsed_ms.
valid must be VALID or INVALID. A damaged packet cannot supply its own trusted identity:
use an authenticated outer envelope or leave it missing. Repeats never increase distinct
counts. Usable on-time wins over late/damaged; late wins over damaged; repair can upgrade.
Never infer clock time from packet presentation timestamps. Invalid args INVALID,
regressing time or a closed window CONFLICT; every failure preserves state.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fvm_smeasure_record(&s->measurement, piece.part, FVM_SMEASURE_VALID, elapsed_ms);
```

[Complete calling context](../../protocol/smeasure.c#L307)

## См. также

- [Implementation: protocol/smeasure.c:95](../../protocol/smeasure.c#L95)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
