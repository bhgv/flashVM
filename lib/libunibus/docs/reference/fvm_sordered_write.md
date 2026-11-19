# fvm_sordered_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordered_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_write(const FVM_SOrdinal *ordinal,
                               unsigned long tag,
                               const void *bytes,
                               size_t size,
                               void *output,
                               size_t capacity,
                               size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `ordinal` | `const FVM_SOrdinal *ordinal` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `bytes` | `const void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sordered_write: checked ordinal prefixes around unchanged S-record bytes.

### Preserved implementation/header contract

EN/RU: Encode prefix plus one original S-record atomically, bounded by original global body limit.
Nonoverlapping borrowed inputs/output, no heap; output/used unchanged on any refusal.
Атомарно кодировать префикс и одну исходную S-запись; буферы не пересекаются, выделения памяти нет.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fvm_sordered_write(&ordinal, tag, bytes, size, output, capacity, used);
```

[Complete calling context](../../order/send.c#L84)

## See also

- [Implementation: protocol/order/packet.c:35](../../protocol/order/packet.c#L35)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
