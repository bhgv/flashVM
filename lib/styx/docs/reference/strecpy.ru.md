# strecpy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](strecpy.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *strecpy(char *to, char *e, char *from);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `to` | `char *to` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `e` | `char *e` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `from` | `char *from` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

strecpy: исходная POSIX-поддержка lib9 strecpy.

### Сохранённый контракт implementation/header

most mem and string routines are declared by ANSI/POSIX files above

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/strecpy.c:21](../../vendor/inferno/lib9/strecpy.c#L21)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
