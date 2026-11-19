# styxorderwait

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderwait.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderwait(StyxOrder *order, unsigned long tag, unsigned timeout);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Live native queue engine; keep alive until this wait and other concurrent calls finish. |
| `tag` | `unsigned long tag` | Only this direction-local tag queue is observed. |
| `timeout` | `unsigned timeout` | Monotonic wait budget in milliseconds, 0..120000; zero tests current readiness. |

## Description

styxorderwait: independent per-tag inbox state.

Returns nonzero only if the selected queue has a ready record. Returns zero on timeout, closed-empty state, invalid arguments or native clock/wait failure. It does not distinguish these zero cases; inspect lifecycle separately. Other tags have independent readiness.

### Preserved implementation/header contract

EN/RU: Block only selected-tag native consumer under one monotonic budget 0..120000ms, 1 ready/0 otherwise.
Ждать только выбранный тег: 1 — готов, 0 — иначе. Без потребления пакета и остановки брокера.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(styxorderwait(owner, 7, 3000));
```

[Complete calling context](../../order/tests/queues.c#L25)

## See also

- [Implementation: order/inbox.c:90](../../order/inbox.c#L90)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
