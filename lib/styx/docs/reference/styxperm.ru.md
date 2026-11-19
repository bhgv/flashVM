# styxperm

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxperm.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxperm(Styxfile *f, char *uid, int mode);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Styxfile *f` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `uid` | `char *uid` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `mode` | `int mode` | Режим операции/доступа; использовать протокольные константы, не суффикс имени. |

## Описание

styxperm: исходная диспетчеризация сессий/fid и callback-операции namespace.

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxperm(Styxfile *file, char *uid, int mode);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L259)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc:184](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc#L184)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
