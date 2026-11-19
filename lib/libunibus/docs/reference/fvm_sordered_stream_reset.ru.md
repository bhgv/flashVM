# fvm_sordered_stream_reset

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordered_stream_reset.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_stream_reset(FVM_SOrderedStream *stream);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `stream` | `FVM_SOrderedStream *stream` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sordered_stream_reset: независимое состояние тегов stream.

### Сохранённый контракт implementation/header

EN/RU: Explicitly discard view/prefix/error without wiping storage; owner first retires borrowed views.
Явно сбросить состояние и ошибку без очистки storage, после завершения использования заимствованных views.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(fvm_sordered_stream_reset(&stream) == FVM_S_OK);
```

[Complete calling context](../../order/tests/codec.c#L83)

## См. также

- [Implementation: protocol/order/stream.c:169](../../protocol/order/stream.c#L169)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
