# fvm_sstream_push

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sstream_push.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_push(FVM_SStream *stream, const void *input, size_t size, FVM_SRecord *view, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `stream` | `FVM_SStream *stream` | Initialized decoder; a ready view requires reset before another push. |
| `input` | `const void *input` | Borrowed new fragment, disjoint from receive buffer/state; NULL only with size zero. |
| `size` | `size_t size` | Accessible bytes in this new fragment, not total bytes retained by the decoder. |
| `view` | `FVM_SRecord *view` | Published only on OK; borrows receive storage until reset/disposal. |
| `used` | `size_t *used` | Consumed fragment bytes, including on MORE/data error; retain only the unconsumed suffix. Invalid arguments leave it unchanged. |

## Description

fvm_sstream_push: incremental checked S-record input and caller-buffer lifetime.

OK yields at most one record. MORE retains consumed input inside stream. Errors latch until reset; calling push after a ready view returns INVALID with used=0. No I/O or blocking is performed; concurrent access to the same decoder needs external synchronization.

### Preserved implementation/header contract

Consume at most one record, stopping before trailing input. Input must not overlap
buffer/state. used is consumed input even on MORE or a latched data error; unchanged on
invalid arguments. view changes only on OK and borrows buffer until reset/disposal.
Preserve/append only bytes not consumed; consumed bytes are already retained. Zero input
returns MORE for an incomplete record. After OK, push returns INVALID with used=0 until
reset; errors are latched with used=0 on retry. Bounds are enforced as soon as framing is
available, before copying an advertised body.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
FVM_SResult result = fvm_sstream_push(&stream, input + cursor, (size_t)amount - cursor, &record, &consumed);
```

[Complete calling context](../../examples/bridge/receive.c#L109)

## See also

- [Implementation: protocol/sstream.c:146](../../protocol/sstream.c#L146)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
