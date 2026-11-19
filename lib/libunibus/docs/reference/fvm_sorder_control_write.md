# fvm_sorder_control_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sorder_control_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sordctl.h>

FVM_SResult fvm_sorder_control_write(const FVM_SOrderControl *message, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `message` | `const FVM_SOrderControl *message` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sorder_control_write: named order-service offer/accept/renewal records.

### Preserved implementation/header contract

EN: Encode two complete nested S-records into disjoint output; no outer tag-zero wrapper.
Identity record tag0, command record tag1..6, six canonical unsigned integers in fixed order.
Borrow message/output; outputs unchanged on any failure, no allocation, C89 ABI.
RU: Кодировать две вложенные S-записи без внешнего tag0; шесть канонических чисел.
Буферы не пересекаются; отказ не меняет output/used, выделения памяти нет.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/order/control.c:92](../../protocol/order/control.c#L92)
- [Public declaration](../../include/flashvm/sordctl.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
