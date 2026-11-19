# fvm_smeasure_media_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smeasure_media_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_media_init(FVM_SMeasureMedia *out,
                                    FVM_SMediaReceiver *receiver,
                                    unsigned long window,
                                    unsigned long deadline_ms);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `FVM_SMeasureMedia *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `receiver` | `FVM_SMediaReceiver *receiver` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `window` | `unsigned long window` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `deadline_ms` | `unsigned long deadline_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

fvm_smeasure_media_init: измерения потока и явные входы адаптации.

### Сохранённый контракт implementation/header

Bind an already initialized receiver with no accepted parts; derive expected from agreed
total/profile. Copies metadata, borrows receiver and its existing data storage until finish.
INVALID for bad descriptors, CONFLICT for a non-pristine receiver; output unchanged.

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

- [Implementation: protocol/smeasure.c:224](../../protocol/smeasure.c#L224)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
