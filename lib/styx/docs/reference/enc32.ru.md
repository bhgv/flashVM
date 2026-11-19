# enc32

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](enc32.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int enc32(char *dest, int ndest, uchar *src, int nsrc);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `dest` | `char *dest` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `ndest` | `int ndest` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `src` | `uchar *src` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `nsrc` | `int nsrc` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

enc32: исходная POSIX-поддержка lib9 u32.

### Сохранённый контракт implementation/header

Encode every tail length, reserving space for the terminating NUL.

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/u32.c:96](../../vendor/inferno/lib9/u32.c#L96)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
