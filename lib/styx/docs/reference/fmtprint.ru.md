# fmtprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fmtprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int fmtprint(Fmt *f, char *fmt, ...);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fmt *f` | Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `...` | `...` | Переменные аргументы, соответствующие переданному format. |

## Описание

fmtprint: исходная POSIX-поддержка lib9 fmtprint.

### Сохранённый контракт implementation/header

format a string into the output buffer
designed for formats which themselves call fmt,
but ignore any width flags

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/fmtprint.c:42](../../vendor/inferno/lib9/fmtprint.c#L42)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
