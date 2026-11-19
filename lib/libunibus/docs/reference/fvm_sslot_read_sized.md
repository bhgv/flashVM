# fvm_sslot_read_sized

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sslot_read_sized.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sslot.h>

FVM_SResult fvm_sslot_read_sized(const void *input, size_t size, size_t slot_bytes, FVM_SRecord *view, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `slot_bytes` | `size_t slot_bytes` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `view` | `FVM_SRecord *view` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sslot_read_sized: checked fixed-slot geometry and compact-record extents.

### Preserved implementation/header contract

Never let corrupted embedded framing cross the externally fixed boundary.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fvm_sslot_read_sized(input, size, profile->slot_bytes, &record, &used);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-001.inc#L261)

## See also

- [Implementation: protocol/sslot.c:107](../../protocol/sslot.c#L107)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
