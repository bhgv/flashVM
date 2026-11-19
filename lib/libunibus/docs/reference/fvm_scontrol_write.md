# fvm_scontrol_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_scontrol_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_write(const FVM_SControlMessage *m, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `m` | `const FVM_SControlMessage *m` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_scontrol_write: correlated per-tag size/profile negotiation state.

### Preserved implementation/header contract

Encode/decode an ordinary compact tag-zero S-record. No padding or implicit state mutation.
First payload integer 2 selects this service; version 1 remains the legacy bindings codec.
Unknown service version/command UNKNOWN, malformed known data INVALID, CRC errors forwarded.
reserved must be zero. Outputs unchanged on failure, read used counts one record only.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/readable_parts/scontrol-c/part-001.inc:278](../../protocol/readable_parts/scontrol-c/part-001.inc#L278)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
