# styxrmfile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxrmfile.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxrmfile(Styxserver *server, Path qid);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `qid` | `Path qid` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxrmfile: исходная диспетчеризация сессий/fid и callback-операции namespace.

## Возвращаемое значение

`int`; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxrmfile(&s, 3);
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L302)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc:47](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc#L47)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
