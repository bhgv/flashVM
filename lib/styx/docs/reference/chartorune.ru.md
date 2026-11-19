# chartorune

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](chartorune.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int chartorune(Rune *rune, char *str);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `rune` | `Rune *rune` | Unicode rune либо буфер rune, отличный от числа UTF-8 байт. |
| `str` | `char *str` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

chartorune: преобразование и проверка UTF-8 byte/rune.

### Сохранённый контракт implementation/header

rune routines

## Возвращаемое значение

`int`; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fmt = (char *)fmt + chartorune(&rune, fmt);
```

[Complete calling context](../../vendor/inferno/lib9/fmt.c#L204)

## См. также

- [Implementation: vendor/inferno/lib9/rune.c:41](../../vendor/inferno/lib9/rune.c#L41)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
