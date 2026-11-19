# fvm_sordered_stream_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordered_stream_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_stream_init(FVM_SOrderedStream *stream, void *storage, size_t capacity, size_t limit);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `stream` | `FVM_SOrderedStream *stream` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `storage` | `void *storage` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `limit` | `size_t limit` | Explicit local quota; permitted ranges and wire/body units are given in the description. |

## Description

fvm_sordered_stream_init: independent per-tag stream state.

### Preserved implementation/header contract

EN/RU: Initialize decoder over borrowed storage, unchanged descriptor on refusal; same body bounds as SStream.
Инициализировать декодер на заимствованном storage с границами SStream; отказ не меняет descriptor.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(fvm_sordered_stream_init(&stream, storage, sizeof(storage), 128) == FVM_S_OK);
```

[Complete calling context](../../order/tests/codec.c#L76)

## See also

- [Implementation: protocol/order/stream.c:51](../../protocol/order/stream.c#L51)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
