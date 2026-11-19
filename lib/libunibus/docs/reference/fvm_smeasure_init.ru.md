# fvm_smeasure_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smeasure_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_init(FVM_SMeasure *out,
                              unsigned long stream,
                              unsigned long epoch,
                              unsigned long window,
                              unsigned long expected,
                              unsigned long deadline_ms);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `FVM_SMeasure *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `stream` | `unsigned long stream` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `epoch` | `unsigned long epoch` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `window` | `unsigned long window` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `expected` | `unsigned long expected` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `deadline_ms` | `unsigned long deadline_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

fvm_smeasure_init: измерения потока и явные входы адаптации.

### Сохранённый контракт implementation/header

Copy nonzero Uint32 stream/epoch/window, expected 1..PARTS and deadline 1..Uint32.
Failure preserves state. No allocation, socket, timer or implicit session adoption.

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

- [Implementation: protocol/smeasure.c:51](../../protocol/smeasure.c#L51)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
