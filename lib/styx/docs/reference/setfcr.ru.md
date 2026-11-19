# setfcr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](setfcr.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void setfcr(ulong fcr);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fcr` | `ulong fcr` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

setfcr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.

### Сохранённый контракт implementation/header

FSR

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
setfcr(FPPDBL | FPRNR | FPINVAL | FPZDIV | FPUNFL | FPOVFL);
```

[Complete calling context](../../vendor/inferno/libmath/FPcontrol-Linux.c#L20)

## См. также

- [Implementation: vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c:22](../../vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
