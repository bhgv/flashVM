# convD2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](convD2M.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

uint convD2M(Dir *d, uchar *buf, uint nbuf);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `d` | `Dir *d` | Входные metadata для одной counted stat-записи. |
| `buf` | `uchar *buf` | Выходной буфер вызывающего; двухбайтовый size prefix не является целой записью. |
| `nbuf` | `uint nbuf` | Ёмкость приёмника в байтах, включая двухбайтовую длину. |

## Описание

Кодировать стандартную Dir-запись; короткий буфер может получить только двухбайтовый prefix требуемой длины.

Для полной записи сначала вызвать sizeD2M. Если буфер содержит минимум2 байта, но целая допустимая запись не помещается, исходный API пишет prefix требуемой длины и возвращает2. Это не целая запись каталога. Ноль — неверная запись либо ёмкость менее2. Directory read должен выдавать только целые записи.

### Сохранённый контракт implementation/header

Preserve short-buffer size reporting only for representable entries.

## Возвращаемое значение

`uint`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
size = convD2M(&row, bytes, sizeof(bytes));
```

[Complete calling context](../../examples/codec.c#L56)

## См. также

- [Implementation: vendor/inferno/lib9/convD2M.c:93](../../vendor/inferno/lib9/convD2M.c#L93)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
