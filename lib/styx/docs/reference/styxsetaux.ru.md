# styxsetaux

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxsetaux.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

int styxsetaux(Styxserver *server, void *aux);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `aux` | `void *aux` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxsetaux: исходная диспетчеризация сессий/fid и callback-операции namespace.

### Сохранённый контракт implementation/header

Borrow current fid context; set transfers non-nil ownership only into an empty slot.
No automatic cloning: unopened walk clones start empty. Rebuild all consumers.

## Возвращаемое значение

`int`; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
check(styxsetaux(&server, id) == 0, "context adoption");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/fid-context.c#L97)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:17](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L17)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
