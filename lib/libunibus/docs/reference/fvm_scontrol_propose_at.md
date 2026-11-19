# fvm_scontrol_propose_at

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_scontrol_propose_at.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_propose_at(FVM_SControlExchange *x,

                                    unsigned long id,
                                    const FVM_SControlLimits *limits,
                                    const FVM_SControlBoundary *boundary,
                                    FVM_SControlMessage *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `x` | `FVM_SControlExchange *x` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `id` | `unsigned long id` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `limits` | `const FVM_SControlLimits *limits` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `boundary` | `const FVM_SControlBoundary *boundary` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `out` | `FVM_SControlMessage *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_scontrol_propose_at: correlated per-tag size/profile negotiation state.

### Preserved implementation/header contract

Boundary-aware offer: nonzero stream/epoch/first object uses OFFER_AT/ACCEPT_AT. The ACK
must echo the complete boundary exactly, not silently move it. All-zero boundary selects
the generic offer behavior. Host chooses a future object and drains/retires old receiver
storage without pausing the overall timeline. Sizes may increase or decrease within ceiling.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_scontrol_propose_at(x, id, limits, &none, out);
```

[Complete calling context](../../protocol/readable_parts/scontrol-c/part-002.inc#L201)

## See also

- [Implementation: protocol/readable_parts/scontrol-c/part-001.inc:433](../../protocol/readable_parts/scontrol-c/part-001.inc#L433)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
