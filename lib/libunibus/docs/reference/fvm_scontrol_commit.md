# fvm_scontrol_commit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_scontrol_commit.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_commit(FVM_SControlExchange *x, const FVM_SControlMessage *ack);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `x` | `FVM_SControlExchange *x` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `ack` | `const FVM_SControlMessage *ack` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_scontrol_commit: correlated per-tag size/profile negotiation state.

### Preserved implementation/header contract

Initiator commits only its correlated bounded ACCEPT; responder commits its exact selection
after queuing that ACK on the reliable control channel. Host quiesces old payload traffic
before switching framing. Exact committed ACK repeats are idempotent. No I/O, timers or
authentication.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:123](../../protocol/readable_parts/scontrol-c/part-002.inc#L123)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
