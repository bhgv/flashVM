# utfrrune

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](utfrrune.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *utfrrune(char *s, long c);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `char *s` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `c` | `long c` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

utfrrune: исходная POSIX-поддержка lib9 utfrrune.

### Сохранённый контракт implementation/header

Invalid rune values must not alias a byte after libc character conversion.

## Возвращаемое значение

`char *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/utfrrune.c:22](../../vendor/inferno/lib9/utfrrune.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
