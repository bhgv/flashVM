# fvm_smeasure_media_accept

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smeasure_media_accept.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smeasure.h>

FVM_SResult
fvm_smeasure_media_accept(FVM_SMeasureMedia *s, const void *input, size_t size, unsigned long elapsed_ms, int *complete);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `FVM_SMeasureMedia *s` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `complete` | `int *complete` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

fvm_smeasure_media_accept: измерения потока и явные входы адаптации.

### Сохранённый контракт implementation/header

Verify slot/identity and accept actual bytes before crediting the decoded part once.
Host supplies monotonic relative arrival time and authenticates the channel. Bad packets
preserve adapter, receiver, image and complete; unidentified damage remains missing.
Late valid packets may complete FILE data before window close without improving on_time.
Closed/regressing/reinitialized-object/profile CONFLICT; codec errors propagate.

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

- [Implementation: protocol/smeasure.c:282](../../protocol/smeasure.c#L282)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
