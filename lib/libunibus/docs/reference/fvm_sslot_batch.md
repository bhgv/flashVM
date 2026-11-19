# fvm_sslot_batch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sslot_batch.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sslot.h>

FVM_SResult
fvm_sslot_batch(const void *input, size_t size, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `notify` | `FVM_SSlotNotify notify` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `userdata` | `void *userdata` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `report` | `FVM_SSlotReport *report` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sslot_batch: checked fixed-slot geometry and compact-record extents.

### Preserved implementation/header contract

Accept zero through BATCH complete slots; partial trailing slot is INVALID before callbacks.
Every structurally bounded batch returns OK even if individual slots are damaged. Inspect
report/callback results. Invalid arguments leave report untouched and invoke no callbacks.

## Return value

`FVM_SResult`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/sslot.c:250](../../protocol/sslot.c#L250)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
