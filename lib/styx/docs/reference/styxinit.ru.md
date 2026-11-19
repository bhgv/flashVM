# styxinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxinit.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

char *styxinit(Styxserver *server, Styxops *ops, char *port, int mode, int needfile);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `ops` | `Styxops *ops` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `port` | `char *port` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `mode` | `int mode` | Режим операции/доступа; использовать протокольные константы, не суффикс имени. |
| `needfile` | `int needfile` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

Инициализировать исходный Styx-сервер, его корень и listener выбранной платформы.

### Сохранённый контракт implementation/header

EN/RU: Legacy constructor cannot silently create a network listener in the PHY backend.
Legacy constructor не создаёт network listener в PHY backend.

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
char *error = styxinit(&server, &ops, argc > 1 ? argv[1] : "0", 0555, 0);
```

[Complete calling context](../../examples/server.c#L364)

## См. также

- [Implementation: server/phy/lifetime.c:111](../../server/phy/lifetime.c#L111)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
