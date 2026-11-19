# fvm_smeasure_media_accept

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smeasure_media_accept.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smeasure.h>

FVM_SResult
fvm_smeasure_media_accept(FVM_SMeasureMedia *s, const void *input, size_t size, unsigned long elapsed_ms, int *complete);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `FVM_SMeasureMedia *s` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `elapsed_ms` | `unsigned long elapsed_ms` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `complete` | `int *complete` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

fvm_smeasure_media_accept: per-stream measurements and explicit adaptation inputs.

### Preserved implementation/header contract

Verify slot/identity and accept actual bytes before crediting the decoded part once.
Host supplies monotonic relative arrival time and authenticates the channel. Bad packets
preserve adapter, receiver, image and complete; unidentified damage remains missing.
Late valid packets may complete FILE data before window close without improving on_time.
Closed/regressing/reinitialized-object/profile CONFLICT; codec errors propagate.

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

- [Implementation: protocol/smeasure.c:282](../../protocol/smeasure.c#L282)
- [Public declaration](../../include/flashvm/smeasure.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
