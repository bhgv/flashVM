# styxend

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxend.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

char *styxend(Styxserver *server);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |

## Описание

Завершить исходный сервер, клиентов и открытые fid через callbacks освобождения.

### Сохранённый контракт implementation/header

EN/RU: Original end API retires deferred reads/fids/clients once, then releases native context owner.
Native producer pins remain wakeable after disconnect until their own release.
Исходный end API завершает reads/fids/clients один раз; producer pins остаются wakeable до release.

## Возвращаемое значение

`char *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxend(&server);
```

[Complete calling context](../../examples/server.c#L377)

## См. также

- [Implementation: server/phy/lifetime.c:286](../../server/phy/lifetime.c#L286)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
