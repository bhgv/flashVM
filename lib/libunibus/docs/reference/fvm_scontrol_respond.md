# fvm_scontrol_respond

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_scontrol_respond.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_respond(FVM_SControlExchange *x, const FVM_SControlMessage *offer, FVM_SControlMessage *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `x` | `FVM_SControlExchange *x` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `offer` | `const FVM_SControlMessage *offer` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `out` | `FVM_SControlMessage *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_scontrol_respond: correlated per-tag size/profile negotiation state.

### Preserved implementation/header contract

Select bounded limits from an authenticated offer and local maxima, store pending selection,
return ACCEPT. Unsupported fixed mode UNKNOWN. No active configuration changes yet.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:55](../../protocol/readable_parts/scontrol-c/part-002.inc#L55)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
