# styxphyinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphyinit.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

char *
styxphyinit(Styxserver *server, Styxops *ops, RemotePhy *phy, int permissions, int needfile, unsigned queued_frames);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `ops` | `Styxops *ops` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `permissions` | `int permissions` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `needfile` | `int needfile` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `queued_frames` | `unsigned queued_frames` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxphyinit: реализация phy/traversal и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Consume connected reliable ordered stream PHY on every outcome; NULL success/error string.
Fresh zeroed server/borrowed ops; native owner serializes init/wait/process/reply/end.
queued_frames1..128 bounds encoded reply storage; no socket/listener/dial/authentication added.
Link infernostyx_phy instead of the original POSIX backend; original 9P API/codecs remain intact.
Ordinary single-session server: never creates P9MX or a reverse client.
Принять connected reliable ordered PHY всегда; NULL success/error string. Server zeroed,
ops borrowed; один native owner. queued_frames1..128, без socket/listener/dial/new authority.
Обычный server одной сессии; не создаёт P9MX или встречного клиента.

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
char *error = styxphyinit(&server, &ops, owner->phy, 0555, 1, 2);
```

[Complete calling context](../../examples/memory_phy/server.c#L52)

## См. также

- [Implementation: server/phy/traversal.c:145](../../server/phy/traversal.c#L145)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
