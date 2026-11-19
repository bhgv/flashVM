# cleanname

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](cleanname.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *cleanname(char *name);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `name` | `char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |

## Описание

cleanname: лексическая нормализация пути без обращения к файловой системе.

### Сохранённый контракт implementation/header

one-of-a-kind

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/cleanname.c:22](../../vendor/inferno/lib9/cleanname.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
