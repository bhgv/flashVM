# styxattachreply

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxattachreply.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxattachreply(StyxAttach *request, char *error);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `request` | `StyxAttach *request` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `error` | `char *error` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxattachreply: реализация libstyx/attach и нативное состояние.

### Сохранённый контракт implementation/header

Borrow live pending client, nil after retirement.

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxattachreply(StyxAttach *, char *error);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L18)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/attach.c:196](../../vendor/inferno/tools/libstyx/attach.c#L196)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
