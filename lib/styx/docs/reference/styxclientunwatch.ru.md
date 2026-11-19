# styxclientunwatch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientunwatch.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientunwatch(StyxWatch *watch);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `watch` | `StyxWatch *watch` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientunwatch: нативные client-операции life и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume watch without waiting; an already admitted callback may finish.
Поглотить watch без ожидания; admitted callback может завершиться позже.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
void styxclientunwatch(StyxWatch *watch);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L132)

## См. также

- [Implementation: client/readable_parts/life-c/part-001.inc:178](../../client/readable_parts/life-c/part-001.inc#L178)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
