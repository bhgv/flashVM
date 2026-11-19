# vfprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](vfprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int vfprint(int fd, char *fmt, va_list args);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fd` | `int fd` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `args` | `va_list args` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

vfprint: исходная POSIX-поддержка lib9 vfprint.

### Сохранённый контракт implementation/header

Dispatch only a valid format, then report final native write failures.

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
n = vfprint(fd, fmt, args);
```

[Complete calling context](../../vendor/inferno/lib9/fprint.c#L40)

## См. также

- [Implementation: vendor/inferno/lib9/vfprint.c:115](../../vendor/inferno/lib9/vfprint.c#L115)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
