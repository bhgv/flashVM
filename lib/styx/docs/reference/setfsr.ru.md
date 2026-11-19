# setfsr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](setfsr.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void setfsr(ulong fsr);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fsr` | `ulong fsr` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

setfsr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
setfsr(0); /* Clear pending exceptions */
```

[Complete calling context](../../vendor/inferno/libmath/FPcontrol-Linux.c#L19)

## См. также

- [Implementation: vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c:97](../../vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c#L97)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
