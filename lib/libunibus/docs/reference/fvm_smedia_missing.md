# fvm_smedia_missing

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_missing.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_missing(
    const FVM_SMediaReceiver *r, unsigned long start, unsigned long *parts, size_t capacity, size_t *count);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `r` | `const FVM_SMediaReceiver *r` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `start` | `unsigned long start` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `parts` | `unsigned long *parts` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `count` | `size_t *count` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |

## Description

fvm_smedia_missing: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Enumerate absent part indices >=start, at most capacity, in ascending order. Continue with
the last returned index+1. FILE reports holes; LIVE always returns count zero. No timer,
network request, retry storm or transfer acknowledgement is generated. Invalid args leave
output/count unchanged. A host bounds/rate-limits requests and retains sender data through
file completion.

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

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:30](../../protocol/readable_parts/smedia-c/part-002.inc#L30)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
