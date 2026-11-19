# styxclientwalkprobe

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwalkprobe.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientwalkprobe(StyxClient *client, StyxFid *base, const char *relative, unsigned operation, const char *detail);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `client` | `StyxClient *client` | Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API. |
| `base` | `StyxFid *base` | Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами. |
| `relative` | `const char *relative` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `operation` | `unsigned operation` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `detail` | `const char *detail` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientwalkprobe: нативные client-операции traversal и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Probe active destination state before crossing a negotiated stream, without pushing it or doing I/O.
Raw profile returns WALK_OK. base/path identify the retained root, operation/detail follow walk.h; WALK_* result.
Проверить active destination до negotiated stream, без push/I/O; raw profile WALK_OK. base/path — retained root.

## Возвращаемое значение

`int`; прямые константы результата тела: WALK_INVALID, WALK_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: client/traversal.c:153](../../client/traversal.c#L153)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
