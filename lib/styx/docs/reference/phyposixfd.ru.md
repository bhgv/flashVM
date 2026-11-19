# phyposixfd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phyposixfd.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/drivers/posix/fd.h>

RemotePhy *phyposixfd(int descriptor, unsigned capabilities);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `descriptor` | `int descriptor` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `capabilities` | `unsigned capabilities` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

phyposixfd: реализация posix/fd и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Consume fd on every outcome; owned physical adapter/NULL, no open/dial/authentication.
Caller explicitly asserts carrier capabilities; fd alone does not prove reliable framing.
Supports O_RDWR sockets and byte devices; configures NONBLOCK/CLOEXEC, preserves other flags.
Host configures serial/bus details before adoption; wake uses a private native pipe.
Принять fd всегда; свой adapter/NULL, без open/dial/auth. Caller явно задаёт capabilities;
O_RDWR socket/byte device, NONBLOCK/CLOEXEC; serial/bus configuration делает host до adoption.

## Возвращаемое значение

`RemotePhy *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), msize, user, name, timeout, limit);
```

[Complete calling context](../../client/fd.c#L37)

## См. также

- [Implementation: remote_phy/drivers/posix/fd.c:317](../../remote_phy/drivers/posix/fd.c#L317)
- [Public declaration](../../remote_phy/drivers/posix/fd.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
