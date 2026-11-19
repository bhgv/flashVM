# fvm_sslot_batch_sized

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sslot_batch_sized.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sslot.h>

FVM_SResult fvm_sslot_batch_sized(
    const void *input, size_t size, size_t slot_bytes, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `slot_bytes` | `size_t slot_bytes` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `notify` | `FVM_SSlotNotify notify` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `userdata` | `void *userdata` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `report` | `FVM_SSlotReport *report` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sslot_batch_sized: checked fixed-slot geometry and compact-record extents.

### Preserved implementation/header contract

Corrupt slots are reported independently; even damaged lengths cannot redirect iteration.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_sslot_batch_sized(input, size, FVM_SSLOT_BYTES, notify, userdata, report);
```

[Complete calling context](../../protocol/sslot.c#L252)

## See also

- [Implementation: protocol/sslot.c:189](../../protocol/sslot.c#L189)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
