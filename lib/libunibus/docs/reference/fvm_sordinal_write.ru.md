# fvm_sordinal_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordinal_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordinal_write(const FVM_SOrdinal *value, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `value` | `const FVM_SOrdinal *value` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sordinal_write: ширины unsigned64 номеров и проверка CRC5.

### Сохранённый контракт implementation/header

EN/RU: Encode canonical prefix, S_OK/LIMIT/INVALID, outputs/used unchanged on failure; no heap, C89 ABI.
Header low3=number_bytes-1, high5=CRC over those3 bits followed by number bytes. MSB first, poly0x05,
init0x1f/xorout0x1f, no reflection or padding. Zero is one zero byte, leading zero bytes forbidden.
Каноническое кодирование префикса: младшие 3 бита — длина минус один, старшие 5 — CRC длины и номера; отказ не меняет результат.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(fvm_sordinal_write(&value, bytes, width, &used) == FVM_S_LIMIT && used == 444);
```

[Complete calling context](../../order/tests/codec.c#L29)

## См. также

- [Implementation: protocol/order/prefix.c:95](../../protocol/order/prefix.c#L95)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
