# styxfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxfree.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

void styxfree(void *p);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `p` | `void *p` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |

## Описание

styxfree: исходная диспетчеризация сессий/fid и callback-операции namespace.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
void styxfree(void *p);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L319)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-001.inc:149](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-001.inc#L149)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
