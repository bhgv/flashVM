# dorfmt

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](dorfmt.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int dorfmt(Fmt *f, Rune *fmt);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |
| `fmt` | `Rune *fmt` | Unicode rune либо буфер rune, отличный от числа UTF-8 байт. |

## Описание

dorfmt: исходная POSIX-поддержка lib9 dorfmt.

### Сохранённый контракт implementation/header

format the output into f->to and return the number of characters fmted

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

- [Implementation: vendor/inferno/lib9/dorfmt.c:36](../../vendor/inferno/lib9/dorfmt.c#L36)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
