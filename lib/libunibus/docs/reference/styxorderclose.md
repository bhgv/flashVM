# styxorderclose

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderclose.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

void styxorderclose(StyxOrder *order);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |

## Description

styxorderclose: independent per-tag life state.

### Preserved implementation/header contract

EN/RU: Close connection owner, wake all native waits, keep ready records individually drainable.
Закрыть соединение и разбудить ожидающих; готовые записи остаются доступны для поштучного чтения.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxorderclose(receiver); /* EN: Invoke styxorderclose with these explicit arguments. RU: Вызвать styxorderclose с этими явными аргументами. */
```

[Complete calling context](../../examples/queues.c#L76)

## See also

- [Implementation: order/life.c:159](../../order/life.c#L159)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
