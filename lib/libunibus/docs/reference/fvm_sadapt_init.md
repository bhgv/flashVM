# fvm_sadapt_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sadapt_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sadapt.h>

FVM_SResult fvm_sadapt_init(FVM_SAdapt *out, const FVM_SAdaptConfig *c);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `FVM_SAdapt *out` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `c` | `const FVM_SAdaptConfig *c` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sadapt_init: host-selected link adaptation decisions and hysteresis.

### Preserved implementation/header contract

Validate/copy complete ladder and policy; errors preserve state and borrow nothing. State
must remain at a stable address after init (config.profiles references its internal table).

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

- [Implementation: protocol/readable_parts/sadapt-c/part-001.inc:170](../../protocol/readable_parts/sadapt-c/part-001.inc#L170)
- [Public declaration](../../include/flashvm/sadapt.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
