# getcallerpc

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](getcallerpc.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

ulong
getcallerpc(void *x);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `x` | `void *x` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

getcallerpc: исходная POSIX-поддержка lib9 getcallerpc-FreeRTOS-riscv64.

### Сохранённый контракт implementation/header

one-of-a-kind

## Возвращаемое значение

`ulong`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/getcallerpc-FreeRTOS-riscv64.c:25](../../vendor/inferno/lib9/getcallerpc-FreeRTOS-riscv64.c#L25)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
