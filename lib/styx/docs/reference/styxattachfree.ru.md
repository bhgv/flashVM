# styxattachfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxattachfree.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

void styxattachfree(StyxAttach *request);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `request` | `StyxAttach *request` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxattachfree: реализация libstyx/attach и нативное состояние.

### Сохранённый контракт implementation/header

1 consumed/attempted,0 retired/staged; nil Rattach.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
void styxattachfree(StyxAttach *); /* Drop caller ref; reply error before abandoning live work. */
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L31)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/attach.c:37](../../vendor/inferno/tools/libstyx/attach.c#L37)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
