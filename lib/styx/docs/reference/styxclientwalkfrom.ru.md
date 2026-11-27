# styxclientwalkfrom

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwalkfrom.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `base` | `StyxFid *base` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `relative` | `const char *relative` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientwalkfrom: нативные client-операции walk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Walk from a borrowed pinned unopened fid; dot clones, owned independent result/NULL.
Serialize base open/clunk against this call; concurrent walks are allowed. File descendants
and unsafe paths refuse before wire effects. No fallback to the original attach pathname.
Пройти от pinned unopened fid; dot клонирует, свой result/NULL; без fallback к attach pathname.

## Возвращаемое значение

`StyxFid *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L192)

## См. также

- [Implementation: client/walk.c:254](../../client/walk.c#L254)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
