# fvm_sordered_stream_push

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordered_stream_push.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_stream_push(
    FVM_SOrderedStream *stream, const void *input, size_t size, FVM_SOrderedRecord *view, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `stream` | `FVM_SOrderedStream *stream` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `view` | `FVM_SOrderedRecord *view` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sordered_stream_push: independent per-tag stream state.

### Preserved implementation/header contract

EN/RU: Consume at most one packet; used reports actual consumed bytes on valid calls including errors.
Input disjoint from state/storage, view changes only on OK, MORE retains fragments. Ready/error is sticky.
Потребить не более одного пакета: used сообщает фактическое число байт, view меняется лишь при OK; фрагменты сохраняются.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(fvm_sordered_stream_push(&stream, bytes, split, &view, &used) == FVM_S_MORE && used == split);
```

[Complete calling context](../../order/tests/codec.c#L77)

## See also

- [Implementation: protocol/order/stream.c:92](../../protocol/order/stream.c#L92)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
