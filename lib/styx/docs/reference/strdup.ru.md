# strdup

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](strdup.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *strdup(const char *s);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `const char *s` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

strdup: исходная POSIX-поддержка lib9 strdup.

### Сохранённый контракт implementation/header

most mem and string routines are declared by ANSI/POSIX files above

## Возвращаемое значение

`char *`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
request->uname = strdup(call->uname[0] ? call->uname : client->uname);
```

[Complete calling context](../../vendor/inferno/tools/libstyx/attach.c#L280)

## См. также

- [Implementation: vendor/inferno/lib9/strdup.c:17](../../vendor/inferno/lib9/strdup.c#L17)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
