# fvm_sadapt_observe

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sadapt_observe.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_observe(FVM_SAdapt *s, const FVM_SAdaptFeedback *f, FVM_SAdaptDecision *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `FVM_SAdapt *s` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `f` | `const FVM_SAdaptFeedback *f` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `out` | `FVM_SAdaptDecision *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sadapt_observe: host-selected link adaptation decisions and hysteresis.

### Preserved implementation/header contract

Consume one validated report atomically. Duplicate/old/wrong identity or pending proposal
CONFLICT; malformed counts INVALID. Unknown RTT forbids growth. Missing+late+damaged all
count as unusable by deadline. INSUFFICIENT resets streaks but does not spend cooldown.
Errors leave state/decision unchanged. A proposal never changes current geometry or sends
control traffic.

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

- [Implementation: protocol/readable_parts/sadapt-c/part-001.inc:222](../../protocol/readable_parts/sadapt-c/part-001.inc#L222)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
