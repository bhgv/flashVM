# runefmtstrinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](runefmtstrinit.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int runefmtstrinit(Fmt *f);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |

## Описание

runefmtstrinit: исходная POSIX-поддержка lib9 vsmprint.

### Сохранённый контракт implementation/header

Initialize an allocated Rune formatter with a reserved terminator slot.

## Возвращаемое значение

`int`; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/vsmprint.c:246](../../vendor/inferno/lib9/vsmprint.c#L246)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
