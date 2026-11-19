# fprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int fprint(int fd, char *fmt, ...);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fd` | `int fd` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `...` | `...` | Переменные аргументы, соответствующие переданному format. |

## Описание

fprint: исходная POSIX-поддержка lib9 fprint.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fprint(2, "backend initialization failed\n");
```

[Complete calling context](../../examples/server.c#L361)

## См. также

- [Implementation: vendor/inferno/lib9/fprint.c:34](../../vendor/inferno/lib9/fprint.c#L34)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
