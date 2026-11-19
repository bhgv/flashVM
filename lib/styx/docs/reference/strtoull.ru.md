# strtoull

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](strtoull.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

uvlong strtoull(const char *nptr, char **endptr, int base);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `nptr` | `const char *nptr` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `endptr` | `char **endptr` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `base` | `int base` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

strtoull: исходная POSIX-поддержка lib9 strtoull.

### Сохранённый контракт implementation/header

Consume all digits and saturate only beyond the full unsigned 64-bit range.

## Возвращаемое значение

`uvlong`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
unsigned long long removed = strtoull(command + 7, &end, 10); /* EN: Parse a numeric object identity and inspect the end pointer. RU: Разобрать числовой ID и проверить end pointer. */
```

[Complete calling context](../../examples/live.c#L199)

## См. также

- [Implementation: vendor/inferno/lib9/strtoull.c:26](../../vendor/inferno/lib9/strtoull.c#L26)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
