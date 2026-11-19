# fvm_sslot_write_sized

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sslot_write_sized.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sslot.h>

FVM_SResult fvm_sslot_write_sized(
    unsigned long tag, const void *payload, size_t size, size_t slot_bytes, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `payload` | `const void *payload` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `slot_bytes` | `size_t slot_bytes` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sslot_write_sized: checked fixed-slot geometry and compact-record extents.

### Preserved implementation/header contract

Explicit-size variants use an already agreed slot extent, MIN_BYTES..MAX_BYTES. They have
the same ownership/atomicity rules as the default 1024-byte wrappers, and allocate nothing.
A size_t unable to represent the negotiated extent must reject it at host negotiation.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_sslot_write_sized(tag, payload, size, FVM_SSLOT_BYTES, output, capacity, used);
```

[Complete calling context](../../protocol/sslot.c#L83)

## See also

- [Implementation: protocol/sslot.c:34](../../protocol/sslot.c#L34)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
