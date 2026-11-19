# fvm_smeasure_media_finish

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smeasure_media_finish.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_media_finish(FVM_SMeasureMedia *s,
                                      unsigned long elapsed_ms,
                                      unsigned long rtt_known,
                                      unsigned long rtt_ms,
                                      FVM_SAdaptFeedback *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `FVM_SMeasureMedia *s` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `rtt_known` | `unsigned long rtt_known` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `rtt_ms` | `unsigned long rtt_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `out` | `FVM_SAdaptFeedback *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_smeasure_media_finish: per-stream measurements and explicit adaptation inputs.

### Preserved implementation/header contract

Freeze the bound window; identical contract to finish. Afterwards release adapter's borrow
and use ordinary media_accept for any later FILE repair, leaving report history unchanged.

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

- [Implementation: protocol/smeasure.c:333](../../protocol/smeasure.c#L333)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
