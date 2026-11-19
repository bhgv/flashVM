# fvm_scontrol_active

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_scontrol_active.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/scontrol.h>

FVM_SResult
fvm_scontrol_active(const FVM_SControlExchange *x, FVM_SControlLimits *limits, FVM_SControlBoundary *boundary);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `x` | `const FVM_SControlExchange *x` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `limits` | `FVM_SControlLimits *limits` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `boundary` | `FVM_SControlBoundary *boundary` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_scontrol_active: correlated per-tag size/profile negotiation state.

### Preserved implementation/header contract

Copy the committed limits/boundary without exposing mutable exchange internals to adapters.
UNKNOWN before any agreement; INVALID for bad native state/outputs. Outputs remain unchanged
on error and must be disjoint from each other and the exchange. Owner-serialized, no I/O.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:222](../../protocol/readable_parts/scontrol-c/part-002.inc#L222)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
