# styxclientlookupfrom

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientlookupfrom.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxFid *styxclientlookupfrom(StyxFid *base, const char *path, StyxCancel *cancel, int *missing);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `base` | `StyxFid *base` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `path` | `const char *path` | Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |
| `missing` | `int *missing` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientlookupfrom: нативные client-операции walk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Same lookup from caller-pinned unopened base; serialize its open/clunk through return.
Тот же lookup от pinned unopened base; open/clunk сериализованы до return.

## Возвращаемое значение

`StyxFid *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxFid *styxclientlookupfrom(StyxFid *base, const char *path, StyxCancel *cancel, int *missing);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L297)

## См. также

- [Implementation: client/walk.c:404](../../client/walk.c#L404)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
