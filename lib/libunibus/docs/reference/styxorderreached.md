# styxorderreached

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderreached.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderreached(StyxOrder *order, unsigned long tag, unsigned long long boundary);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `boundary` | `unsigned long long boundary` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxorderreached: independent per-tag inbox state.

### Preserved implementation/header contract

EN/RU: Non-consuming old-prefix checkpoint,1/0; packet merely held/ready is not considered consumed.
Native host uses this per-tag watermark before applying assignment requests across lanes.
Проверить отметку выданного номера тега: 1/0. Записи в held/ready ещё не выданы; host использует отметку при переназначении.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(styxorderreached(order, 7, 0) && !styxorderreached(order, 7, 1));
```

[Complete calling context](../../order/tests/queues.c#L86)

## See also

- [Implementation: order/inbox.c:142](../../order/inbox.c#L142)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
