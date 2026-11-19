# runevsmprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](runevsmprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

Rune *runevsmprint(char *fmt, va_list args);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `args` | `va_list args` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

runevsmprint: исходная POSIX-поддержка lib9 vsmprint.

### Сохранённый контракт implementation/header

Format directly in Rune mode so counts and custom conversions retain their semantics.

## Возвращаемое значение

`Rune *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
p = runevsmprint(fmt, args);
```

[Complete calling context](../../vendor/inferno/lib9/runesmprint.c#L28)

## См. также

- [Implementation: vendor/inferno/lib9/vsmprint.c:281](../../vendor/inferno/lib9/vsmprint.c#L281)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
