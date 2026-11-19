# styxclientclunklater

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientclunklater.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientclunklater(StyxFid *fid);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |

## Описание

styxclientclunklater: нативные client-операции clunk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume fid without RPC wait; native worker owns client/number only.
Setup failure fails the connection, never leaks caller ownership or waits for a guest.
Поглотить fid без RPC wait; worker владеет client/number; setup failure завершает connection.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxclientclunklater(fid);
```

[Complete calling context](../../client/clunk.c#L63)

## См. также

- [Implementation: client/clunk.c:21](../../client/clunk.c#L21)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
