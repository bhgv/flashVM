# fvm_smeasure_finish

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smeasure_finish.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_finish(
    FVM_SMeasure *s, unsigned long elapsed_ms, unsigned long rtt_known, unsigned long rtt_ms, FVM_SAdaptFeedback *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `FVM_SMeasure *s` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `rtt_known` | `unsigned long rtt_known` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `rtt_ms` | `unsigned long rtt_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `out` | `FVM_SAdaptFeedback *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_smeasure_finish: измерения потока и явные входы адаптации.

### Сохранённый контракт implementation/header

Close once, at/after deadline and last arrival; fill owned copied report atomically.
RTT known is 0/1; unknown requires zero RTT. No automatic probe or RTT inference.
Premature/regressing/already closed CONFLICT; invalid RTT/args INVALID. Later repair
cannot improve this historical report. Missing = expected-on_time-late-damaged.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_smeasure_finish(&s->measurement, elapsed_ms, rtt_known, rtt_ms, out);
```

[Complete calling context](../../protocol/smeasure.c#L343)

## См. также

- [Implementation: protocol/smeasure.c:161](../../protocol/smeasure.c#L161)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
