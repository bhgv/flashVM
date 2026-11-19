# styxreadfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxreadfree.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

void styxreadfree(StyxRead *request);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `request` | `StyxRead *request` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxreadfree: реализация libstyx/read и нативное состояние.

### Сохранённый контракт implementation/header

Consume caller ref on owner; freeing a live handle alone does not answer/cancel it.
Поглотить caller ref на owner; само освобождение не отвечает/не отменяет запрос.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
void styxreadfree(StyxRead *);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L175)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/read.c:38](../../vendor/inferno/tools/libstyx/read.c#L38)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
