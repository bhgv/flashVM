# getfcr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](getfcr.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

ulong getfcr(void);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| — | void | Параметров нет. |

## Описание

getfcr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.

## Возвращаемое значение

`ulong`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
vv = (getfcr() & ~FPFCR) | (v & FPFCR);
```

[Complete calling context](../../vendor/inferno/lib9/setfcr-Solaris-sparc.c#L42)

## См. также

- [Implementation: vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c:45](../../vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c#L45)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
