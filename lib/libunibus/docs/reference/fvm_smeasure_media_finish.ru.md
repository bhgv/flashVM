# fvm_smeasure_media_finish

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smeasure_media_finish.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_media_finish(FVM_SMeasureMedia *s,
                                      unsigned long elapsed_ms,
                                      unsigned long rtt_known,
                                      unsigned long rtt_ms,
                                      FVM_SAdaptFeedback *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `FVM_SMeasureMedia *s` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `rtt_known` | `unsigned long rtt_known` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `rtt_ms` | `unsigned long rtt_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `out` | `FVM_SAdaptFeedback *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_smeasure_media_finish: измерения потока и явные входы адаптации.

### Сохранённый контракт implementation/header

Freeze the bound window; identical contract to finish. Afterwards release adapter's borrow
and use ordinary media_accept for any later FILE repair, leaving report history unchanged.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/smeasure.c:333](../../protocol/smeasure.c#L333)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
