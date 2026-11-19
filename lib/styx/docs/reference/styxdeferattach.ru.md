# styxdeferattach

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxdeferattach.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

StyxAttach *styxdeferattach(Styxserver *server, void *aux, void (*release)(void *));
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `aux` | `void *aux` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `void (*release)(void *)` | `void (*release)(void *)` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxdeferattach: реализация libstyx/attach и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Socket-owner root selection, pending NULL uses current attach;0/-1, no authority grant.
Выбор root на socket-owner; pending NULL — текущий attach;0/-1, без выдачи authority.

## Возвращаемое значение

`StyxAttach *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxAttach *styxdeferattach(Styxserver *, void *aux, void (*release)(void *));
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L420)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/attach.c:95](../../vendor/inferno/tools/libstyx/attach.c#L95)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
