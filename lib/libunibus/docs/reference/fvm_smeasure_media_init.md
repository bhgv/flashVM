# fvm_smeasure_media_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smeasure_media_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_media_init(FVM_SMeasureMedia *out,
                                    FVM_SMediaReceiver *receiver,
                                    unsigned long window,
                                    unsigned long deadline_ms);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `FVM_SMeasureMedia *out` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `receiver` | `FVM_SMediaReceiver *receiver` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `window` | `unsigned long window` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `deadline_ms` | `unsigned long deadline_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

fvm_smeasure_media_init: per-stream measurements and explicit adaptation inputs.

### Preserved implementation/header contract

Bind an already initialized receiver with no accepted parts; derive expected from agreed
total/profile. Copies metadata, borrows receiver and its existing data storage until finish.
INVALID for bad descriptors, CONFLICT for a non-pristine receiver; output unchanged.

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

- [Implementation: protocol/smeasure.c:224](../../protocol/smeasure.c#L224)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
