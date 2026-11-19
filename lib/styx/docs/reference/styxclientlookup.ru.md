# styxclientlookup

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientlookup.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxFid *styxclientlookup(StyxClient *client, const char *path, StyxCancel *cancel, int *missing);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `path` | `const char *path` | Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав. |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |
| `missing` | `int *missing` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientlookup: нативные client-операции walk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Root-relative lookup with per-call copied absence, owned fid/NULL; missing required.
Borrow client/path/cancel through return. Missing1 only exact acknowledged Inferno
Enonexist under explicit profile; all other failures leave missing0. Inferno lookup
walks one component per RPC so a partial Rwalk cannot erase the failure reason.
Native PHY or legacy supported; non-NULL cancel requires PHY owner. No data retry.
Root-relative lookup, свой fid/NULL, required missing;1 только explicit Inferno
Enonexist, прочие ошибки0. Inputs borrowed, по одному компоненту под Inferno profile.

## Возвращаемое значение

`StyxFid *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxFid *styxclientlookup(StyxClient *client, const char *path, StyxCancel *cancel, int *missing);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L274)

## См. также

- [Implementation: client/walk.c:366](../../client/walk.c#L366)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
