# styxorderencode

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderencode.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

FVM_SResult styxorderencode(
    StyxOrder *order, unsigned long tag, const void *bytes, size_t size, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `bytes` | `const void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

styxorderencode: independent per-tag send state.

### Preserved implementation/header contract

EN/RU: Encode one packet into caller-owned disjoint spool, advance TX only on OK; no transport I/O.
Returned packet must be retained/retried as the SAME bytes until sent or connection retired, not re-encoded.
Pending rollover/MAX exhaustion returns CONFLICT for only this tag. Output/used unchanged on failure.
Кодировать пакет в буфер caller; TX меняется только при OK. Для повторной передачи сохранять те же байты, не назначать новый номер.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int good = styxorderencode(sender, 8, "eight", 5, packet, sizeof(packet), &used) == FVM_S_OK && /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
```

[Complete calling context](../../examples/queues.c#L68)

## See also

- [Implementation: order/send.c:62](../../order/send.c#L62)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
