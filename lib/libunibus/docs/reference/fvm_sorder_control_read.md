# fvm_sorder_control_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sorder_control_read.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sordctl.h>

FVM_SResult fvm_sorder_control_read(const void *input, size_t size, FVM_SOrderControl *message);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `message` | `FVM_SOrderControl *message` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sorder_control_read: named order-service offer/accept/renewal records.

### Preserved implementation/header contract

EN: Read exactly one named service body, copying scalars only on OK. UNKNOWN for other identities.
Known malformed messages return INVALID/checksum; trailing records/integers are invalid.
RU: Прочитать тело именованной службы; скаляры меняются лишь при OK, другая служба — UNKNOWN.
Ошибка известной службы и лишние данные отвергаются. Input заимствован до возврата.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/order/control.c:150](../../protocol/order/control.c#L150)
- [Public declaration](../../include/flashvm/sordctl.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
