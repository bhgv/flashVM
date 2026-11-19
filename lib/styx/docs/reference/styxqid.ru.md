# styxqid

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxqid.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

Qid styxqid(Path path, int isdir);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `path` | `Path path` | Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав. |
| `isdir` | `int isdir` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

Составить скалярный Qid из стабильного ID backend и признака каталога.

## Возвращаемое значение

`Qid`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
out->qid = styxqid(row->id, (row->mode & CATALOG_DIR) != 0);
```

[Complete calling context](../../examples/server.c#L56)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:355](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L355)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
