# exits

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](exits.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void exits(char *s);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `char *s` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

exits: исходная POSIX-поддержка lib9 exits.

### Сохранённый контракт implementation/header

of loved one & descendants

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
exits(buf);
```

[Complete calling context](../../vendor/inferno/lib9/sysfatal.c#L32)

## См. также

- [Implementation: vendor/inferno/lib9/exits.c:17](../../vendor/inferno/lib9/exits.c#L17)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
