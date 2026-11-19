# pow10

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](pow10.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

double pow10(int n);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `n` | `int n` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

pow10: исходная POSIX-поддержка lib9 pow10.

### Сохранённый контракт implementation/header

Preserve reciprocal subnormals and bound work without negating INT_MIN.

## Возвращаемое значение

`double`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
h = f * pow10(-d);
```

[Complete calling context](../../vendor/inferno/lib9/fltfmt.c#L174)

## См. также

- [Implementation: vendor/inferno/lib9/pow10.c:47](../../vendor/inferno/lib9/pow10.c#L47)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
