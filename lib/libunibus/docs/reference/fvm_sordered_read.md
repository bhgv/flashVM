# fvm_sordered_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordered_read.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_read(const void *input, size_t size, size_t limit, FVM_SOrderedRecord *view, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `limit` | `size_t limit` | Explicit local quota; permitted ranges and wire/body units are given in the description. |
| `view` | `FVM_SOrderedRecord *view` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sordered_read: checked ordinal prefixes around unchanged S-record bytes.

### Preserved implementation/header contract

EN/RU: Validate prefix and exactly one S-record, limit applies to S-record body; trailing packets remain unread.
Borrow input through view lifetime; view/used unchanged on MORE/error, original CRC still checked.
Проверить префикс и одну S-запись; следующие пакеты не потребляются, MORE/ошибка не меняют view/used.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(fvm_sordered_read(bytes, size, 128, &view, &used) == FVM_S_CHECKSUM && used == 555);
```

[Complete calling context](../../order/tests/codec.c#L121)

## See also

- [Implementation: protocol/order/packet.c:92](../../protocol/order/packet.c#L92)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
