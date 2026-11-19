# fvm_smedia_accept

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_smedia_accept.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_accept(FVM_SMediaReceiver *r, const void *input, size_t size, int *complete);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `r` | `FVM_SMediaReceiver *r` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `complete` | `int *complete` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

fvm_smedia_accept: bounded media-fragment framing and caller-owned reassembly.

### Preserved implementation/header contract

Verify before copying; reject mismatched identity CONFLICT without changing
state/buffer/complete. Exact duplicate OK without increasing received; same part with
different valid bytes CONFLICT. Copy only verified new part. complete is true once every part
arrived, not a playback decision. Caller authenticates packet/session before entry; CRC alone
cannot establish trust.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fvm_smedia_accept(s->receiver, input, size, &done);
```

[Complete calling context](../../protocol/smeasure.c#L301)

## See also

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:399](../../protocol/readable_parts/smedia-c/part-001.inc#L399)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
