# styxorderfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderfree.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

void styxorderfree(StyxOrder *order);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |

## Description

styxorderfree: independent per-tag life state.

### Preserved implementation/header contract

EN/RU: Consume after all feed/encode/consumer calls retire; NULL harmless, no owned worker to join.
Освободить после завершения всех вызовов; NULL безопасен, собственных workers нет.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxorderfree(context.receiver);
```

[Complete calling context](../../examples/queues.c#L152)

## See also

- [Implementation: order/life.c:210](../../order/life.c#L210)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
