# fvm_sstream_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sstream_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_init(FVM_SStream *stream, void *buffer, size_t capacity, size_t limit);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `stream` | `FVM_SStream *stream` | Writable decoder state for one receive direction; initialize before push. |
| `buffer` | `void *buffer` | Caller-owned receive storage, disjoint from stream and later input fragments; retain through decoder use. |
| `capacity` | `size_t capacity` | Accessible bytes in buffer; may impose a tighter bound than limit. |
| `limit` | `size_t limit` | Maximum body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX. |

## Description

fvm_sstream_init: incremental checked S-record input and caller-buffer lifetime.

### Preserved implementation/header contract

Initialize without touching buffer; capacity may be below limit and then enforces a tighter
bound. No allocation, locks, VM/GC or I/O. Output/storage must not overlap.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fvm_sstream_init(&next.stream, storage, capacity, limit);
```

[Complete calling context](../../protocol/order/stream.c#L60)

## See also

- [Implementation: protocol/sstream.c:84](../../protocol/sstream.c#L84)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
