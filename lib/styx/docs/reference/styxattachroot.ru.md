# styxattachroot

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxattachroot.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxattachroot(Styxserver *server, StyxAttach *pending, Qid qid);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `pending` | `StyxAttach *pending` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `qid` | `Qid qid` | Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути. |

## Описание

styxattachroot: реализация libstyx/attach и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Socket-owner root selection, pending NULL uses current attach;0/-1, no authority grant.
Выбор root на socket-owner; pending NULL — текущий attach;0/-1, без выдачи authority.

## Возвращаемое значение

`int`; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxattachroot(Styxserver *server, StyxAttach *pending, Qid qid);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L403)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/attach.c:152](../../vendor/inferno/tools/libstyx/attach.c#L152)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
