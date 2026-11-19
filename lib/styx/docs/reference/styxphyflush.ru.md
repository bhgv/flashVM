# styxphyflush

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphyflush.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

int styxphyflush(Styxserver *server, unsigned timeout);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `timeout` | `unsigned timeout` | Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом. |

## Описание

styxphyflush: реализация phy/io и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner flushes staged output within one monotonic budget0..120000ms;1 drained/0 pending/failure.
Partial known prefix is retained, uncertain output terminally fails; no automatic replay/reconnect.
Native wait only, not guest cooperation. Explicit styxend discards any remaining output.
Owner flush с единым budget0..120000ms;1 drained/0 pending/failure, tail сохраняется,
uncertain terminal, без replay/reconnect. styxend явно отбрасывает остаток.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: server/phy/io.c:246](../../server/phy/io.c#L246)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
