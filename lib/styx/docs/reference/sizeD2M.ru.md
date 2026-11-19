# sizeD2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](sizeD2M.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

uint sizeD2M(Dir *d);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `d` | `Dir *d` | Входные metadata каталога; строки должны быть допустимыми C-строками с NUL. |

## Описание

Вычислить полный размер стандартной stat-записи, включая начальное поле длины.

Результат включает начальный двухбайтовый count. У строк собственные uint16 lengths; слишком длинные строки/запись дают ноль. Offset каталога увеличивается на полный кодированный размер, не на количество детей.

### Сохранённый контракт implementation/header

Report zero when the stat size cannot be represented on the wire.

## Возвращаемое значение

`uint`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
uint required = sizeD2M(&info);
```

[Complete calling context](../../examples/server.c#L208)

## См. также

- [Implementation: vendor/inferno/lib9/convD2M.c:69](../../vendor/inferno/lib9/convD2M.c#L69)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
