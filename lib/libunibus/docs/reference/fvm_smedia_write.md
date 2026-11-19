# fvm_smedia_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_write(unsigned long tag,
                             const FVM_SMediaInfo *info,
                             unsigned long part,
                             const void *data,
                             size_t size,
                             void *out,
                             size_t capacity,
                             size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `info` | `const FVM_SMediaInfo *info` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `part` | `unsigned long part` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `data` | `const void *data` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `out` | `void *out` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_smedia_write: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Encode one exact part to one padded slot. tag must be nonzero Uint32. Atomic output/used on
failure; payload must have precisely the extent implied by total/part. No allocation or I/O.

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

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:85](../../protocol/readable_parts/smedia-c/part-002.inc#L85)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
