# styxphywakefree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphywakefree.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

void styxphywakefree(StyxPhyWake *context);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `context` | `StyxPhyWake *context` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxphywakefree: реализация phy/lifetime и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Consume readiness pin without waiting for server/guest; NULL harmless.
Поглотить readiness pin без ожидания server/guest; NULL harmless.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxphywakefree(context);
```

[Complete calling context](../../server/phy/lifetime.c#L186)

## См. также

- [Implementation: server/phy/lifetime.c:21](../../server/phy/lifetime.c#L21)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
