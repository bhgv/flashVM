# styxattachclient

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxattachclient.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

Client *styxattachclient(StyxAttach *request);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `request` | `StyxAttach *request` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxattachclient: реализация libstyx/attach и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Borrow live pending client only on its owner thread, nil otherwise.
Заимствовать live pending client только на owner-thread, иначе nil.

## Возвращаемое значение

`Client *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
Client *styxattachclient(StyxAttach *); /* Borrow live pending client, nil after retirement. */
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L433)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/attach.c:128](../../vendor/inferno/tools/libstyx/attach.c#L128)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
