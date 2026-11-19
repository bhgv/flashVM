# styxorderstart

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderstart.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderstart(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long first);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `direction` | `unsigned direction` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `first` | `unsigned long long first` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxorderstart: independent per-tag life state.

### Preserved implementation/header contract

EN/RU: Native-only seed an unused directional counter before peer traffic,1/0; default starts at0.
Existing/used direction cannot be overwritten. Caller agrees initial value with peer; no inferred wire command.
Задать first для неиспользованного tag/direction: 1/0; по умолчанию ноль. Начальное значение явно согласуется с другой стороной.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(styxorderstart(order, 7, STYX_ORDER_RX, 0));
```

[Complete calling context](../../order/tests/fault.c#L88)

## See also

- [Implementation: order/life.c:119](../../order/life.c#L119)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
