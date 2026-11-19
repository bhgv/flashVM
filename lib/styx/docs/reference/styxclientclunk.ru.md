# styxclientclunk

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientclunk.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientclunk(StyxFid *fid);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |

## Описание

styxclientclunk: нативные client-операции fid и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume one fid, clunk if transport usable, then drop its connection pin.
Поглотить fid, clunk если transport пригоден; NULL допустим.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxclientclunk(fid); /* EN: Invoke styxclientclunk with these explicit arguments. RU: Вызвать styxclientclunk с этими явными аргументами. */
```

[Complete calling context](../../examples/client.c#L84)

## См. также

- [Implementation: client/fid.c:21](../../client/fid.c#L21)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
