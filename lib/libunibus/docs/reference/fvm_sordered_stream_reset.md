# fvm_sordered_stream_reset

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordered_stream_reset.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_stream_reset(FVM_SOrderedStream *stream);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `stream` | `FVM_SOrderedStream *stream` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sordered_stream_reset: independent per-tag stream state.

### Preserved implementation/header contract

EN/RU: Explicitly discard view/prefix/error without wiping storage; owner first retires borrowed views.
Явно сбросить состояние и ошибку без очистки storage, после завершения использования заимствованных views.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(fvm_sordered_stream_reset(&stream) == FVM_S_OK);
```

[Complete calling context](../../order/tests/codec.c#L83)

## See also

- [Implementation: protocol/order/stream.c:169](../../protocol/order/stream.c#L169)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
