# convM2D

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](convM2D.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

uint convM2D(uchar *buf, uint nbuf, Dir *d, char *strs);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `uchar *buf` | Входные counted stat-байты; native layout Dir не принимается. |
| `nbuf` | `uint nbuf` | Доступная входная длина в байтах. |
| `d` | `Dir *d` | Приёмник декодированных scalar fields и указателей строк. |
| `strs` | `char *strs` | Буфер декодированных строк либо NULL для пустых string fields. Для допустимой записи достаточно nbuf байт. |

## Описание

Декодировать проверенную Dir-запись и скопировать строки в буфер вызывающего, если он задан.

Вход ограничен nbuf. В strs нужен собственный буфер для всех строк с терминаторами; nbuf байт достаточно для проверенной записи. NULL strs запрашивает скалярные metadata с пустыми строками. Codec строки не выделяет. Возвращается размер одной записи; ноль отклоняет неверный/усечённый вход.

### Сохранённый контракт implementation/header

Decode only the declared first record, leaving outputs untouched on bad input.

## Возвращаемое значение

`uint`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
check(convM2D((uchar *)out.data, out.count, &d, strings) == out.count, "partial record");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L239)

## См. также

- [Implementation: vendor/inferno/lib9/convM2D.c:72](../../vendor/inferno/lib9/convM2D.c#L72)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
