# seek

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](seek.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

vlong seek(int fd, vlong where, int from);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fd` | `int fd` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `where` | `vlong where` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `from` | `int from` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

seek: исходная POSIX-поддержка lib9 seek.

### Сохранённый контракт implementation/header

Keep the full signed offset when calling the Windows CRT seek entry point.

## Возвращаемое значение

`vlong`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
previous = seek(fd, 0, 1);
```

[Complete calling context](../../vendor/inferno/lib9/pread-Nt.c#L37)

## См. также

- [Implementation: vendor/inferno/lib9/seek.c:26](../../vendor/inferno/lib9/seek.c#L26)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
