# fvm_sslot_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sslot_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sslot.h>

FVM_SResult
fvm_sslot_write(unsigned long tag, const void *payload, size_t size, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `payload` | `const void *payload` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sslot_write: checked fixed-slot geometry and compact-record extents.

### Preserved implementation/header contract

One complete record followed by zero padding; input/output/state must not overlap.
No allocation, transport, VM or tag binding. Writer publishes exactly SSLOT_BYTES on OK.
The slot profile/size must be agreed over a trusted control channel, not inferred from
damage.

## Return value

`FVM_SResult`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/sslot.c:81](../../protocol/sslot.c#L81)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
