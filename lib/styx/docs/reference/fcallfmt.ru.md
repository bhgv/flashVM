# fcallfmt

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fcallfmt.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <fcall.h>

int fcallfmt(Fmt *fmt);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fmt` | `Fmt *fmt` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |

## Описание

fcallfmt: исходная POSIX-поддержка lib9 fcallfmt.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/fcallfmt.c:37](../../vendor/inferno/lib9/fcallfmt.c#L37)
- [Public declaration](../../vendor/inferno/include/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
