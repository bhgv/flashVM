# fvm_smeasure_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smeasure_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_init(FVM_SMeasure *out,
                              unsigned long stream,
                              unsigned long epoch,
                              unsigned long window,
                              unsigned long expected,
                              unsigned long deadline_ms);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `FVM_SMeasure *out` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `stream` | `unsigned long stream` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `epoch` | `unsigned long epoch` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `window` | `unsigned long window` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `expected` | `unsigned long expected` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `deadline_ms` | `unsigned long deadline_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

fvm_smeasure_init: per-stream measurements and explicit adaptation inputs.

### Preserved implementation/header contract

Copy nonzero Uint32 stream/epoch/window, expected 1..PARTS and deadline 1..Uint32.
Failure preserves state. No allocation, socket, timer or implicit session adoption.

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

- [Implementation: protocol/smeasure.c:51](../../protocol/smeasure.c#L51)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
