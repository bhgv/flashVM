# cistrncmp

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](cistrncmp.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int cistrncmp(char *s1, char *s2, int n);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s1` | `char *s1` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `s2` | `char *s2` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `n` | `int n` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

cistrncmp: исходная POSIX-поддержка lib9 cistrncmp.

### Сохранённый контракт implementation/header

most mem and string routines are declared by ANSI/POSIX files above

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

- [Implementation: vendor/inferno/lib9/cistrncmp.c:24](../../vendor/inferno/lib9/cistrncmp.c#L24)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
