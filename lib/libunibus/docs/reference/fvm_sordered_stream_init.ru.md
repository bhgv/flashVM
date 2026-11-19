# fvm_sordered_stream_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordered_stream_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_stream_init(FVM_SOrderedStream *stream, void *storage, size_t capacity, size_t limit);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `stream` | `FVM_SOrderedStream *stream` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `storage` | `void *storage` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `limit` | `size_t limit` | Явная локальная квота; диапазон и единицы wire/body указаны в описании. |

## Описание

fvm_sordered_stream_init: независимое состояние тегов stream.

### Сохранённый контракт implementation/header

EN/RU: Initialize decoder over borrowed storage, unchanged descriptor on refusal; same body bounds as SStream.
Инициализировать декодер на заимствованном storage с границами SStream; отказ не меняет descriptor.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(fvm_sordered_stream_init(&stream, storage, sizeof(storage), 128) == FVM_S_OK);
```

[Complete calling context](../../order/tests/codec.c#L76)

## См. также

- [Implementation: protocol/order/stream.c:51](../../protocol/order/stream.c#L51)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
