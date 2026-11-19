# styxorderreceivewindow

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderreceivewindow.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderreceivewindow(StyxOrder *order, unsigned long long window);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `window` | `unsigned long long window` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxorderreceivewindow: independent per-tag life state.

### Preserved implementation/header contract

EN: Tighten receive displacement before first RX data, 1/0; equal window remains idempotent.
Borrow engine; native peer has agreed this bound. Existing TX counters are untouched.
RU: Сузить окно приёма до первых RX-данных: 1/0, равное окно идемпотентно. TX не меняется.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: order/life.c:246](../../order/life.c#L246)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
