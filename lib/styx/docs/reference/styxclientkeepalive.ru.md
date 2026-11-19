# styxclientkeepalive

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientkeepalive.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `idle` | `unsigned idle` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `reply` | `unsigned reply` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `misses` | `unsigned misses` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxclientkeepalive: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Configure carrier-owner heartbeat without protocol wait;1/0. All zeros disables.
idle/reply1..120000ms, misses2..16; NULL/legacy/terminal/invalid refuses unchanged.
One probe retains its tags/deadline through reconfiguration or disabling.
Настроить carrier-owner heartbeat без ожидания;1/0, три нуля отключают будущие probes.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L316)

## См. также

- [Implementation: client/fd.c:128](../../client/fd.c#L128)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
