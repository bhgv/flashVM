# styxdeferread

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxdeferread.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

StyxRead *styxdeferread(Styxserver *server, void *aux, void (*release)(void *));
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

styxdeferread: реализация libstyx/read и нативное состояние.

### Сохранённый контракт implementation/header

Native owner-thread byte-read extension, no new wire messages. Defer only during
read callback, once per non-directory/nonzero read and at most one per fid.
Set *n=0 and return nil on success. Returned request is caller-owned; NULL
leaves aux caller-owned. At most128 pending reads per client. No buffers retained.
Worker threads pass copied data through native mailboxes/fds, never request pointers.
Native byte-read extension; только read callback, *n=0/nil при успехе, свой request.
Worker передаёт copied data через mailbox/fd; request доступен только owner-thread.

## Возвращаемое значение

`StyxRead *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxRead *styxdeferread(Styxserver *, void *aux, void (*release)(void *));
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L75)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/read.c:100](../../vendor/inferno/tools/libstyx/read.c#L100)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
