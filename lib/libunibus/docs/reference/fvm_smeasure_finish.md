# fvm_smeasure_finish

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smeasure_finish.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smeasure.h>

FVM_SResult fvm_smeasure_finish(
    FVM_SMeasure *s, unsigned long elapsed_ms, unsigned long rtt_known, unsigned long rtt_ms, FVM_SAdaptFeedback *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `FVM_SMeasure *s` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `rtt_known` | `unsigned long rtt_known` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `rtt_ms` | `unsigned long rtt_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `out` | `FVM_SAdaptFeedback *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_smeasure_finish: per-stream measurements and explicit adaptation inputs.

### Preserved implementation/header contract

Close once, at/after deadline and last arrival; fill owned copied report atomically.
RTT known is 0/1; unknown requires zero RTT. No automatic probe or RTT inference.
Premature/regressing/already closed CONFLICT; invalid RTT/args INVALID. Later repair
cannot improve this historical report. Missing = expected-on_time-late-damaged.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_smeasure_finish(&s->measurement, elapsed_ms, rtt_known, rtt_ms, out);
```

[Complete calling context](../../protocol/smeasure.c#L343)

## See also

- [Implementation: protocol/smeasure.c:161](../../protocol/smeasure.c#L161)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
