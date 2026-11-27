# styxcancelcreate

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxcancelcreate.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxCancel *styxcancelcreate(void);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| — | void | Параметров нет. |

## Описание

styxcancelcreate: нативные client-операции cancel и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Owned sticky cancellation token/NULL; request is nonwaiting and thread-safe.
One admitted operation per token; admitted operation retains it independently.
Caller can free after admission; freeing alone does not request cancellation.
Свой sticky token/NULL; request thread-safe и без ожидания, один operation/token.

## Возвращаемое значение

`StyxCancel *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxCancel *child = styxcancelcreate();
```

[Complete calling context](../../client/cancel.c#L243)

## См. также

- [Implementation: client/cancel.c:37](../../client/cancel.c#L37)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
