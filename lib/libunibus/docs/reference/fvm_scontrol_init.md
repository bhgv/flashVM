# fvm_scontrol_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_scontrol_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_init(FVM_SControlExchange *out, const FVM_SControlLimits *ceiling);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `FVM_SControlExchange *out` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `ceiling` | `const FVM_SControlLimits *ceiling` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_scontrol_init: correlated per-tag size/profile negotiation state.

### Preserved implementation/header contract

Copy local maxima; compact ceiling has zero slot/part, fixed ceiling may also accept compact
offers. Native host must really provide these capacities and authenticate service messages.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/readable_parts/scontrol-c/part-001.inc:398](../../protocol/readable_parts/scontrol-c/part-001.inc#L398)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
