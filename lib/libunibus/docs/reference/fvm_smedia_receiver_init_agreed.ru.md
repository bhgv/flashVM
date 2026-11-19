# fvm_smedia_receiver_init_agreed

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_receiver_init_agreed.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_receiver_init_agreed(FVM_SMediaReceiver *out,
                                            const FVM_SControlExchange *agreement,
                                            const FVM_SMediaInfo *info,
                                            void *buffer,
                                            size_t capacity);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `FVM_SMediaReceiver *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `agreement` | `const FVM_SControlExchange *agreement` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `info` | `const FVM_SMediaInfo *info` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `buffer` | `void *buffer` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |

## Описание

fvm_smedia_receiver_init_agreed: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Initialize from committed limits and activation boundary; rejects objects before the agreed
first object or from another stream/epoch. Host retires old receive buffers at the segment
boundary; no timeline stop or timer is introduced. Generic agreements require host
quiescence.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:169](../../protocol/readable_parts/smedia-c/part-002.inc#L169)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
