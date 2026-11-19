# fvm_sordered_stream_push

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordered_stream_push.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_stream_push(
    FVM_SOrderedStream *stream, const void *input, size_t size, FVM_SOrderedRecord *view, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `stream` | `FVM_SOrderedStream *stream` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `view` | `FVM_SOrderedRecord *view` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sordered_stream_push: независимое состояние тегов stream.

### Сохранённый контракт implementation/header

EN/RU: Consume at most one packet; used reports actual consumed bytes on valid calls including errors.
Input disjoint from state/storage, view changes only on OK, MORE retains fragments. Ready/error is sticky.
Потребить не более одного пакета: used сообщает фактическое число байт, view меняется лишь при OK; фрагменты сохраняются.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(fvm_sordered_stream_push(&stream, bytes, split, &view, &used) == FVM_S_MORE && used == split);
```

[Complete calling context](../../order/tests/codec.c#L77)

## См. также

- [Implementation: protocol/order/stream.c:92](../../protocol/order/stream.c#L92)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
