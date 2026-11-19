# qsort

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](qsort.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void qsort(void *va, long n, long es, int (*cmp)(void *, void *));
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `va` | `void *va` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `n` | `long n` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |
| `es` | `long es` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `int (*cmp)(void *, void *)` | `int (*cmp)(void *, void *)` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

qsort: исходная POSIX-поддержка lib9 qsort.

### Сохранённый контракт implementation/header

Ignore empty/invalid spans before partitioning or multiplying native counts.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/qsort.c:220](../../vendor/inferno/lib9/qsort.c#L220)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
