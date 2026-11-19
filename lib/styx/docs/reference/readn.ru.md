# readn

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](readn.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

long readn(int f, void *av, long n);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `int f` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `av` | `void *av` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `n` | `long n` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

readn: исходная POSIX-поддержка lib9 readn.

## Возвращаемое значение

`long`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/readn.c:21](../../vendor/inferno/lib9/readn.c#L21)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
