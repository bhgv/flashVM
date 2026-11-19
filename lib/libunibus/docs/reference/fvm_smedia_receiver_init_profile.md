# fvm_smedia_receiver_init_profile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_receiver_init_profile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_receiver_init_profile(FVM_SMediaReceiver *out,
                                             const FVM_SMediaProfile *profile,
                                             unsigned long tag,
                                             const FVM_SMediaInfo *info,
                                             void *buffer,
                                             size_t capacity);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `FVM_SMediaReceiver *out` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `profile` | `const FVM_SMediaProfile *profile` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `info` | `const FVM_SMediaInfo *info` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `buffer` | `void *buffer` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |

## Description

fvm_smedia_receiver_init_profile: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Replace identity/bitmap only after all caller-owned buffer and capacity checks pass.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_smedia_receiver_init_profile(out, &media_default, tag, info, buffer, capacity);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-002.inc#L145)

## See also

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:331](../../protocol/readable_parts/smedia-c/part-001.inc#L331)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
