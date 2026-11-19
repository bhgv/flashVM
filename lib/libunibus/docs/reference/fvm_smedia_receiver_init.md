# fvm_smedia_receiver_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_receiver_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_receiver_init(
    FVM_SMediaReceiver *out, unsigned long tag, const FVM_SMediaInfo *info, void *buffer, size_t capacity);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `FVM_SMediaReceiver *out` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `info` | `const FVM_SMediaInfo *info` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `buffer` | `void *buffer` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |

## Description

fvm_smedia_receiver_init: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Validate expected identity and caller capacity before replacing native state; buffer bytes
are untouched. Reinitialization explicitly abandons the previous object at a host-chosen
deadline. All descriptors, buffers and slot input/output must be disjoint. No cleanup
allocation exists.

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

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:142](../../protocol/readable_parts/smedia-c/part-002.inc#L142)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
