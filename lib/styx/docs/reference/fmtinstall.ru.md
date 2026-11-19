# fmtinstall

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fmtinstall.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int fmtinstall(int c, Fmts f);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `c` | `int c` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `f` | `Fmts f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |

## Описание

fmtinstall: исходный реестр formatter и публикация нативных callbacks.

### Сохранённый контракт implementation/header

EN: Validate c/f and publish one registry entry; callback code runs elsewhere.
Return the original success/failure status, preserving NULL's bad-format fallback.
RU: Проверить c/f и опубликовать запись; callback выполняется снаружи.
Вернуть исходный статус, сохранив замену NULL на bad-format callback.

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fmtinstall('q', quotestrfmt);
```

[Complete calling context](../../vendor/inferno/lib9/fmtquote.c#L426)

## См. также

- [Implementation: vendor/inferno/lib9/fmt.c:153](../../vendor/inferno/lib9/fmt.c#L153)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
