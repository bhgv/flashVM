# fvm_sadapt_commit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sadapt_commit.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_commit(FVM_SAdapt *s, const FVM_SControlLimits *selected);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `FVM_SAdapt *s` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `selected` | `const FVM_SControlLimits *selected` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sadapt_commit: host-selected link adaptation decisions and hysteresis.

### Preserved implementation/header contract

Confirm the exact pending ladder geometry only after the external control ACK committed.
Clamped/different ACK CONFLICT; host rejects/rebuilds ladder for unsupported selections.
Successful commit starts holdoff; no payload/frame reinterpretation is performed here.

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

- [Implementation: protocol/readable_parts/sadapt-c/part-001.inc:314](../../protocol/readable_parts/sadapt-c/part-001.inc#L314)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
