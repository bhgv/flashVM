# styxprocess

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxprocess.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

char *styxprocess(Styxserver *server);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |

## Описание

Диспетчеризовать доступные исходные Styx-сообщения через callbacks сервера.

### Сохранённый контракт implementation/header

EN/RU: Original process API scopes owner-local close tokens; hardware output remains nonblocking.
Исходный process API задаёт owner-local close context; hardware output nonblocking.

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxprocess(&server);
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/session.c#L446)

## См. также

- [Implementation: server/phy/lifetime.c:214](../../server/phy/lifetime.c#L214)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
