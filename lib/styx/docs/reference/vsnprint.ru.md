# vsnprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](vsnprint.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int vsnprint(char *buf, int len, char *fmt, va_list args);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `char *buf` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `len` | `int len` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |
| `fmt` | `char *fmt` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `args` | `va_list args` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

vsnprint: исходная POSIX-поддержка lib9 vsnprint.

### Сохранённый контракт implementation/header

Reject invalid output spans before pointer formation or formatter side effects.

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
n = vsnprint(buf, len, fmt, args);
```

[Complete calling context](../../vendor/inferno/lib9/snprint.c#L42)

## См. также

- [Implementation: vendor/inferno/lib9/vsnprint.c:40](../../vendor/inferno/lib9/vsnprint.c#L40)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
