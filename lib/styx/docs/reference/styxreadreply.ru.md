# styxreadreply

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxreadreply.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxreadreply(StyxRead *request, void *bytes, ulong count, char *error);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `request` | `StyxRead *request` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `bytes` | `void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `count` | `ulong count` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |
| `error` | `char *error` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxreadreply: реализация libstyx/read и нативное состояние.

### Сохранённый контракт implementation/header

Complete once; borrowed bytes copied during send, nil error means Rread.
Invalid count/buffer leaves request pending and returns0; inactive/staged returns0.
Return1 consumes live transaction/attempts response, not peer acknowledgement.
release(aux) runs once after detachment, before fid/client cleanup; never wait or
reenter dispatch/teardown. It may free the caller request ref. Flush/reset/disconnect
cancel without late replies; fid retirement sends Rerror before its clunk reply.
Завершить один раз; bytes borrowed до возврата.1 attempt,0 inactive/invalid/staged.
release(aux) один раз после detach до fid/client cleanup; без ожидания/reentry.

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxreadreply(StyxRead *, void *bytes, ulong count, char *error);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L159)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/read.c:175](../../vendor/inferno/tools/libstyx/read.c#L175)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
