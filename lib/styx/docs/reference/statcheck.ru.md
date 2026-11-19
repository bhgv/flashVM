# statcheck

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](statcheck.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

int statcheck(uchar *buf, uint nbuf);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `uchar *buf` | Counted stat-запись для структурной проверки. |
| `nbuf` | `uint nbuf` | Доступная длина; должна содержать целую counted запись. |

## Описание

Проверить целую stat-запись и каждую counted string, не выдавая частично разобранную запись.

Вернуть0 для целой допустимой stat-записи, -1 при неверных размерах/counts. Это структурная проверка, не решение rwx/auth и не проверка существования объекта backend.

### Сохранённый контракт implementation/header

Validate every counted string before advancing beyond the available record.

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/convM2D.c:23](../../vendor/inferno/lib9/convM2D.c#L23)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
