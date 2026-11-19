# fvm_sordinal_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordinal_read.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordinal_read(const void *input, size_t size, FVM_SOrdinal *value, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `value` | `FVM_SOrdinal *value` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sordinal_read: ширины unsigned64 номеров и проверка CRC5.

### Сохранённый контракт implementation/header

EN/RU: Decode one canonical prefix, MORE until complete; outputs/used unchanged except on OK.
CRC is a corruption check, not authentication or a guarantee that prefix belongs to another valid body.
Прочитать префикс; MORE до завершения. Результат и used меняются лишь при OK; CRC не удостоверяет отправителя.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(fvm_sordinal_read(bytes, length, &result, &read) == FVM_S_MORE);
```

[Complete calling context](../../order/tests/codec.c#L35)

## См. также

- [Implementation: protocol/order/prefix.c:153](../../protocol/order/prefix.c#L153)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
