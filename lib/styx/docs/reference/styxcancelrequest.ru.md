# styxcancelrequest

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxcancelrequest.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxcancelrequest(StyxCancel *cancel);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `cancel` | `StyxCancel *cancel` | Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом. |

## Описание

styxcancelrequest: нативные client-операции cancel и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Request sticky cancellation from a retained native capability; no protocol wait.
Запросить sticky cancellation с retained capability; без protocol wait.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxcancelrequest(cancel->child);
```

[Complete calling context](../../client/cancel.c#L71)

## См. также

- [Implementation: client/cancel.c:89](../../client/cancel.c#L89)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
