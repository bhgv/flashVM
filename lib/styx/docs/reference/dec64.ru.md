# dec64

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](dec64.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int dec64(uchar *out, int lim, char *in, int n);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `uchar *out` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `lim` | `int lim` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `in` | `char *in` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `n` | `int n` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

dec64: исходная POSIX-поддержка lib9 u64.

### Сохранённый контракт implementation/header

Decode tolerated base64 input without touching zero-capacity destinations.

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

- [Implementation: vendor/inferno/lib9/u64.c:50](../../vendor/inferno/lib9/u64.c#L50)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
