# seprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](seprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *seprint(char *buf, char *e, char *fmt, ...);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `char *buf` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `e` | `char *e` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `...` | `...` | Переменные аргументы, соответствующие переданному format. |

## Описание

seprint: исходная POSIX-поддержка lib9 seprint.

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
seprint(buf, e, "Tversion tag %ud msize %ud version '%s'", tag, f->msize, f->version);
```

[Complete calling context](../../vendor/inferno/lib9/fcallfmt.c#L54)

## См. также

- [Implementation: vendor/inferno/lib9/seprint.c:36](../../vendor/inferno/lib9/seprint.c#L36)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
