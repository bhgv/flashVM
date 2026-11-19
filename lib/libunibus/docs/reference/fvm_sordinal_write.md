# fvm_sordinal_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordinal_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordinal_write(const FVM_SOrdinal *value, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `value` | `const FVM_SOrdinal *value` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sordinal_write: unsigned64 ordinal widths and CRC5 validation.

### Preserved implementation/header contract

EN/RU: Encode canonical prefix, S_OK/LIMIT/INVALID, outputs/used unchanged on failure; no heap, C89 ABI.
Header low3=number_bytes-1, high5=CRC over those3 bits followed by number bytes. MSB first, poly0x05,
init0x1f/xorout0x1f, no reflection or padding. Zero is one zero byte, leading zero bytes forbidden.
Каноническое кодирование префикса: младшие 3 бита — длина минус один, старшие 5 — CRC длины и номера; отказ не меняет результат.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(fvm_sordinal_write(&value, bytes, width, &used) == FVM_S_LIMIT && used == 444);
```

[Complete calling context](../../order/tests/codec.c#L29)

## See also

- [Implementation: protocol/order/prefix.c:95](../../protocol/order/prefix.c#L95)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
