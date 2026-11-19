# fvm_sordered_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordered_read.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_read(const void *input, size_t size, size_t limit, FVM_SOrderedRecord *view, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `limit` | `size_t limit` | Явная локальная квота; диапазон и единицы wire/body указаны в описании. |
| `view` | `FVM_SOrderedRecord *view` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sordered_read: проверяемые префиксы номеров вокруг неизменённых S-record байт.

### Сохранённый контракт implementation/header

EN/RU: Validate prefix and exactly one S-record, limit applies to S-record body; trailing packets remain unread.
Borrow input through view lifetime; view/used unchanged on MORE/error, original CRC still checked.
Проверить префикс и одну S-запись; следующие пакеты не потребляются, MORE/ошибка не меняют view/used.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(fvm_sordered_read(bytes, size, 128, &view, &used) == FVM_S_CHECKSUM && used == 555);
```

[Complete calling context](../../order/tests/codec.c#L121)

## См. также

- [Implementation: protocol/order/packet.c:92](../../protocol/order/packet.c#L92)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
