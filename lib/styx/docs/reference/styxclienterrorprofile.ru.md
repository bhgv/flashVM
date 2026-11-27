# styxclienterrorprofile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclienterrorprofile.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclienterrorprofile(StyxClient *client, unsigned profile);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `profile` | `unsigned profile` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxclienterrorprofile: нативные client-операции walk и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Select known error vocabulary before sharing client or allocating walk fids;1/0.
No I/O/authentication, unknown profile/late change refuses. Default is OPAQUE.
Host attests server vocabulary, not access rights; partial walks never imply absence.
Выбрать словарь ошибок до sharing/walk;1/0, без I/O/auth. Default OPAQUE;
unknown/late change refused, partial walk не означает absence.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclienterrorprofile(StyxClient *client, unsigned profile);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L246)

## См. также

- [Implementation: client/walk.c:336](../../client/walk.c#L336)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
