# styxnotifyfd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxnotifyfd.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxnotifyfd(Styxserver *server, int descriptor);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `descriptor` | `int descriptor` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxnotifyfd: реализация phy/backend и нативное состояние.

### Сохранённый контракт implementation/header

POSIX borrowed readiness fd: owner registers fd or -1 to remove;0/-1.
Caller drains and closes it after unregister/end. Other ports return -1.
POSIX borrowed fd готовности; owner регистрирует, сам читает и закрывает.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxnotifyfd(Styxserver *, int fd);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L50)

## См. также

- [Implementation: server/phy/backend.c:302](../../server/phy/backend.c#L302)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
