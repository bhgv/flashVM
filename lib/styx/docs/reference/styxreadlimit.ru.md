# styxreadlimit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxreadlimit.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

ulong styxreadlimit(StyxRead *request);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `request` | `StyxRead *request` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxreadlimit: реализация libstyx/read и нативное состояние.

### Сохранённый контракт implementation/header

Borrow pending client/fid aux and exact capacity/offset only on native socket owner.
Client/aux become nil on retirement; zero limit then distinguishes inactivity.
Заимствовать client/fid aux, capacity/offset на owner; после retirement nil/zero.

## Возвращаемое значение

`ulong`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
ulong styxreadlimit(StyxRead *);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L118)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/read.c:184](../../vendor/inferno/tools/libstyx/read.c#L184)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
