# styxordernext

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxordernext.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxordernext(
    StyxOrder *order, unsigned long tag, unsigned long long *ordinal, void *bytes, size_t capacity, size_t *size);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `ordinal` | `unsigned long long *ordinal` | Output sequence of the consumed record; unchanged when no record is consumed. |
| `bytes` | `void *bytes` | Caller-owned destination, or NULL when capacity is zero. |
| `capacity` | `size_t capacity` | Destination bytes; SMALL preserves the queued record for retry. |
| `size` | `size_t *size` | Required/actual payload bytes when a record is present, including SMALL. |

## Description

styxordernext: independent per-tag inbox state.

RECORD consumes exactly one ready item; SMALL publishes its required size without removing it. EMPTY means no ready item on an open engine; END means closed/invalid without an available item. Ready items can still be drained after close. This call does not wait for arrival.

### Preserved implementation/header contract

EN/RU: Copy one ready selected-tag record/SMALL/EMPTY/END; successful pop advances consumed checkpoint.
One consumer per tag, output/ordinal unchanged except successful copy; SMALL changes only size.
Выдать одну готовую запись выбранного тега и отметить выданный номер; SMALL меняет только size, не потребляя запись.

## Return value

`int`; direct result constants in this body: STYX_ORDER_END. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(styxordernext(order, 7, &ordinal, &tiny, 1, &size) == STYX_ORDER_SMALL);
```

[Complete calling context](../../order/tests/queues.c#L89)

## See also

- [Implementation: order/inbox.c:32](../../order/inbox.c#L32)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
