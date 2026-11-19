# fvm_sinteger_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sinteger_read.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_sinteger_read(const unsigned char *in, size_t size, FVM_SInteger *value, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `in` | `const unsigned char *in` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `value` | `FVM_SInteger *value` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sinteger_read: canonical integers, bounded S-record framing and checksum validation.

### Preserved implementation/header contract

Canonical big-endian signed magnitude. Nonoverlapping outputs required. No allocation/VM.
Write/read value and used only on OK; null input is valid only when size is zero (MORE).
Write needs valid value/output/used. Read needs value/used. Maximum magnitude is 2^64-1.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
FVM_SResult result = fvm_sinteger_read(*cursor, *left, &number, &used);
```

[Complete calling context](../../protocol/sbindings.c#L30)

## See also

- [Implementation: protocol/srecord.c:101](../../protocol/srecord.c#L101)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
