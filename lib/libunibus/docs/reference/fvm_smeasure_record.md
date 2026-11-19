# fvm_smeasure_record

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smeasure_record.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_record(FVM_SMeasure *s, unsigned long part, unsigned int valid, unsigned long elapsed_ms);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `FVM_SMeasure *s` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `part` | `unsigned long part` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `valid` | `unsigned int valid` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

fvm_smeasure_record: per-stream measurements and explicit adaptation inputs.

### Preserved implementation/header contract

Record one trusted zero-based part identity and host-relative monotonic elapsed_ms.
valid must be VALID or INVALID. A damaged packet cannot supply its own trusted identity:
use an authenticated outer envelope or leave it missing. Repeats never increase distinct
counts. Usable on-time wins over late/damaged; late wins over damaged; repair can upgrade.
Never infer clock time from packet presentation timestamps. Invalid args INVALID,
regressing time or a closed window CONFLICT; every failure preserves state.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
fvm_smeasure_record(&s->measurement, piece.part, FVM_SMEASURE_VALID, elapsed_ms);
```

[Complete calling context](../../protocol/smeasure.c#L307)

## See also

- [Implementation: protocol/smeasure.c:95](../../protocol/smeasure.c#L95)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
