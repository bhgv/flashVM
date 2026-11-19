# styxphycycleerror

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphycycleerror.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

char *styxphycycleerror(Styxserver *server);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |

## Описание

styxphycycleerror: реализация phy/traversal и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Return a standard Rerror text while explicitly marking this currently executing callback's result as a cycle.
Server owner/current synchronous callback only; does not grant access, pause a guest, or guess from error text.
Use only when the actual VFS result is DVM_VFS_CYCLE. Raw peers receive ordinary Rerror; profile peers also get a typed flag.
Вернуть Rerror text и явно отметить cycle текущего synchronous callback. Только server owner; без guesses/authority.
Вызывать лишь для фактического DVM_VFS_CYCLE; raw peer получает обычный Rerror, profile также typed flag.

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
reply.ename = styxphycycleerror(server);
```

[Complete calling context](../../server/phy/traversal.c#L197)

## См. также

- [Implementation: server/phy/traversal.c:165](../../server/phy/traversal.c#L165)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
