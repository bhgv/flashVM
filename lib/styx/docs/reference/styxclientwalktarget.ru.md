# styxclientwalktarget

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwalktarget.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientwalktarget(StyxClient *client, WalkEntity *root);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `root` | `WalkEntity *root` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientwalktarget: нативные client-операции traversal и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Copy acknowledged fixed server projection identity,1/0; no I/O or new authority, inputs borrowed.
Скопировать acknowledged server projection identity,1/0; без I/O/new authority, inputs borrowed.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientwalktarget(StyxClient *client, WalkEntity *root);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L168)

## См. также

- [Implementation: client/traversal.c:79](../../client/traversal.c#L79)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
