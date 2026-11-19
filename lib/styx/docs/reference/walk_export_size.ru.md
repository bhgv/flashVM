# walk_export_size

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](walk_export_size.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/walk/walk.h>

size_t walk_export_size(void);
```

## Библиотека

Styx::walk; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| — | void | Параметров нет. |

## Описание

walk_export_size: реализация walk/wire и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Exact exportable current history size including header;0 if any entry has a local-only domain.
Native owner only, no allocation/I/O. Точный размер exportable current history;0 при local-only domain.

## Возвращаемое значение

`size_t`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::walk; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
size_t measured = kind == WALK_PACKET_REPLY ? 4 : walk_export_size();
```

[Complete calling context](../../walk/packet.c#L94)

## См. также

- [Implementation: walk/wire.c:75](../../walk/wire.c#L75)
- [Public declaration](../../walk/walk.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
