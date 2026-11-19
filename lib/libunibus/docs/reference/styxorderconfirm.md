# styxorderconfirm

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderconfirm.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderconfirm(
    StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation, int accepted);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `direction` | `unsigned direction` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `operation` | `unsigned long long operation` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `accepted` | `int accepted` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxorderconfirm: independent per-tag rollover state.

### Preserved implementation/header contract

EN/RU: Apply matching accepted ACK(1)/explicit NACK(0),1/0; exact most-recent repeats idempotent.
ACK resets only this directional counter to0. No automatic wrap; wrong/not-ready confirmation changes nothing.
Native host authenticates session/operation and retires old lane deliveries before accepting rollover.
Точное ACK сбрасывает лишь этот tag/direction в ноль: 1/0. Ошибочное либо преждевременное подтверждение ничего не меняет.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(!styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
```

[Complete calling context](../../order/tests/fault.c#L96)

## See also

- [Implementation: order/rollover.c:172](../../order/rollover.c#L172)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
