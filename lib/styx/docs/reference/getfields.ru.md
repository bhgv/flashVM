# getfields

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](getfields.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int getfields(char *str, char **args, int max, int mflag, char *set);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `str` | `char *str` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `args` | `char **args` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `max` | `int max` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `mflag` | `int mflag` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `set` | `char *set` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

getfields: исходная POSIX-поддержка lib9 getfields.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/getfields.c:25](../../vendor/inferno/lib9/getfields.c#L25)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
