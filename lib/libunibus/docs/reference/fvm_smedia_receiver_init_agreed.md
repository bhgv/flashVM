# fvm_smedia_receiver_init_agreed

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_receiver_init_agreed.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_receiver_init_agreed(FVM_SMediaReceiver *out,
                                            const FVM_SControlExchange *agreement,
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
| `agreement` | `const FVM_SControlExchange *agreement` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `info` | `const FVM_SMediaInfo *info` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `buffer` | `void *buffer` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |

## Description

fvm_smedia_receiver_init_agreed: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Initialize from committed limits and activation boundary; rejects objects before the agreed
first object or from another stream/epoch. Host retires old receive buffers at the segment
boundary; no timeline stop or timer is introduced. Generic agreements require host
quiescence.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:169](../../protocol/readable_parts/smedia-c/part-002.inc#L169)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
