# fvm_smedia_read_profile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_read_profile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_read_profile(
    const FVM_SMediaProfile *profile, const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `profile` | `const FVM_SMediaProfile *profile` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `out` | `FVM_SMediaPiece *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_smedia_read_profile: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Decode without accepting tag declarations from a lossy/untrusted media datagram.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fvm_smedia_read_profile(&r->profile, input, size, r->tag, &piece);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-001.inc#L409)

## See also

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:248](../../protocol/readable_parts/smedia-c/part-001.inc#L248)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
