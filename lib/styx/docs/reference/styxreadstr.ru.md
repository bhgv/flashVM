# styxreadstr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxreadstr.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

long styxreadstr(ulong off, char *buf, ulong n, char *str);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `off` | `ulong off` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `buf` | `char *buf` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `n` | `ulong n` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |
| `str` | `char *str` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxreadstr: исходная диспетчеризация сессий/fid и callback-операции namespace.

## Возвращаемое значение

`long`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
*count = offset >= (vlong)(sizeof(contents) - 1) ? 0 : styxreadstr(offset, bytes, *count, (char *)contents);
```

[Complete calling context](../../examples/memory_phy/server.c#L37)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:326](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L326)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
