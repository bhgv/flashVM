# styxordercreate

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxordercreate.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

StyxOrder *styxordercreate(unsigned limit, unsigned long long window);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `limit` | `unsigned limit` | Explicit local quota; permitted ranges and wire/body units are given in the description. |
| `window` | `unsigned long long window` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxordercreate: independent per-tag life state.

### Preserved implementation/header contract

EN/RU: Owned engine/NULL, no worker or transport; body limit 1..S_BODY_MAX, reorder window 1..2^63-1.
Host feeds validated lanes on native broker threads; clients wait for their own ready tag.
Вернуть собственный engine либо NULL, без worker/transport; limit ограничивает тело записи, window — перестановку номеров, а не длину очереди клиента.

## Return value

`StyxOrder *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxOrder *sender = styxordercreate(256, 16); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
```

[Complete calling context](../../examples/queues.c#L57)

## See also

- [Implementation: order/life.c:63](../../order/life.c#L63)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
