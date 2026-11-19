# rerrstr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](rerrstr.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void rerrstr(char *buf, uint nbuf);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `char *buf` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `nbuf` | `uint nbuf` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

rerrstr: исходная POSIX-поддержка lib9 rerrstr.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
rerrstr(buf, sizeof buf);
```

[Complete calling context](../../vendor/inferno/lib9/errfmt.c#L35)

## См. также

- [Implementation: vendor/inferno/lib9/rerrstr.c:19](../../vendor/inferno/lib9/rerrstr.c#L19)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
