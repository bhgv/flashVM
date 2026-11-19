# fvm_sstream_reset

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sstream_reset.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_reset(FVM_SStream *stream);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `stream` | `FVM_SStream *stream` | Initialized decoder to reuse; invalidates the previous borrowed view and clears latched errors. |

## Description

fvm_sstream_reset: incremental checked S-record input and caller-buffer lifetime.

### Preserved implementation/header contract

Invalidate an outstanding view and discard prefix/error without wiping caller bytes.
Owner must quiesce users first; this does not resynchronize a transport or cancel remote
work.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fvm_sstream_reset(&stream);
```

[Complete calling context](../../examples/nested.c#L126)

## See also

- [Implementation: protocol/sstream.c:113](../../protocol/sstream.c#L113)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
