# styxorderprepare

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderprepare.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderprepare(
    StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation, unsigned long long last);
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
| `last` | `unsigned long long last` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxorderprepare: independent per-tag rollover state.

### Preserved implementation/header contract

EN/RU: Native queued-reset proposal for ONE counter; strictly increasing nonzero local operation ID.
TX boundary must equal last spooled ordinal. RX boundary may wait for its old prefix to arrive/be consumed.
1/0, no wire traffic; host queues service request after old packets and ACKs only when ready returns1.
Подготовить сброс одного счётчика: 1/0. Host сам передаёт служебные request/ACK; остальные теги не затронуты.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(styxorderprepare(order, 7, STYX_ORDER_RX, 1, 1));
```

[Complete calling context](../../order/tests/fault.c#L93)

## See also

- [Implementation: order/rollover.c:89](../../order/rollover.c#L89)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
