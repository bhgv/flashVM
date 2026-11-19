# fvm_sordinal_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sordinal_read.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordinal_read(const void *input, size_t size, FVM_SOrdinal *value, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `value` | `FVM_SOrdinal *value` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sordinal_read: unsigned64 ordinal widths and CRC5 validation.

### Preserved implementation/header contract

EN/RU: Decode one canonical prefix, MORE until complete; outputs/used unchanged except on OK.
CRC is a corruption check, not authentication or a guarantee that prefix belongs to another valid body.
Прочитать префикс; MORE до завершения. Результат и used меняются лишь при OK; CRC не удостоверяет отправителя.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
CHECK(fvm_sordinal_read(bytes, length, &result, &read) == FVM_S_MORE);
```

[Complete calling context](../../order/tests/codec.c#L35)

## See also

- [Implementation: protocol/order/prefix.c:153](../../protocol/order/prefix.c#L153)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
