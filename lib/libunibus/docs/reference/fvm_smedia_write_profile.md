# fvm_smedia_write_profile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_write_profile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_write_profile(const FVM_SMediaProfile *profile,
                                     unsigned long tag,
                                     const FVM_SMediaInfo *info,
                                     unsigned long part,
                                     const void *data,
                                     size_t size,
                                     void *output,
                                     size_t capacity,
                                     size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `profile` | `const FVM_SMediaProfile *profile` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `info` | `const FVM_SMediaInfo *info` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `part` | `unsigned long part` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `data` | `const void *data` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_smedia_write_profile: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Explicit-profile variants use already agreed geometry, preserve the default wrappers and
require no object-sized scratch allocation. Total remains bounded by MAX_BYTES and PARTS.
Profile changes require a quiesced/new object; a data packet cannot change the profile.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_smedia_write_profile(&media_default, tag, info, part, data, size, out, capacity, used);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-002.inc#L94)

## See also

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:185](../../protocol/readable_parts/smedia-c/part-001.inc#L185)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
