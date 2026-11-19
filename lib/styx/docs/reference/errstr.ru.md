# errstr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](errstr.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int errstr(char *buf, uint nerr);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `char *buf` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `nerr` | `uint nerr` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

errstr: исходная POSIX-поддержка lib9 errstr-Nt.

### Сохранённый контракт implementation/header

of loved one & descendants

## Возвращаемое значение

`int`; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
errstr(buf, nerr);
```

[Complete calling context](../../vendor/inferno/lib9/errstr-Plan9.c#L22)

## См. также

- [Implementation: vendor/inferno/lib9/errstr-Nt.c:96](../../vendor/inferno/lib9/errstr-Nt.c#L96)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
