# fvm_srecord_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_srecord_read.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_srecord_read(const void *input, size_t size, size_t limit, FVM_SRecord *view, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Borrowed input bytes; preserve them throughout use of the returned record view. |
| `size` | `size_t size` | Accessible input bytes; trailing records may remain unconsumed. |
| `limit` | `size_t limit` | Allowed body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX. |
| `view` | `FVM_SRecord *view` | Verified borrowed record, published only on OK. |
| `used` | `size_t *used` | Bytes of exactly one record on OK; unchanged on any other result. |

## Description

fvm_srecord_read: canonical integers, bounded S-record framing and checksum validation.

### Preserved implementation/header contract

limit is 1..BODY_MAX and counts tag+payload; pass BODY_MAX explicitly for the full profile.
MORE means retain/retry the prefix. Checksum is verified before tag interpretation.
OK publishes one record and consumed extent; trailing records remain unconsumed.
Caller must initialize neither view nor used; both remain unchanged on non-OK.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int invalid = fvm_srecord_read(record.payload, record.size, BRIDGE_LIMIT + 32, &request, &used) != FVM_S_OK;
```

[Complete calling context](../../examples/bridge/receive.c#L43)

## See also

- [Implementation: protocol/srecord.c:389](../../protocol/srecord.c#L389)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
