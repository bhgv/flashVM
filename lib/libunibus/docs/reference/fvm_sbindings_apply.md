# fvm_sbindings_apply

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sbindings_apply.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_sbindings_apply(FVM_SBindings *table, const void *input, size_t size, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `table` | `FVM_SBindings *table` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sbindings_apply: direction-local numeric tag to schema identity bindings.

### Preserved implementation/header contract

Decode/checksum one complete tag-zero record and atomically append all new associations.
Exact repeats are idempotent; rebinding a tag returns CONFLICT, duplicates in one record
INVALID. Unknown payload version is INVALID. Input/outputs/table/slots must not overlap.
Failed/incomplete records leave every slot/count/used unchanged. No VM execution occurs.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
fvm_sbindings_apply(bindings, encoded, size, &used) == FVM_S_OK; /* EN: Invoke fvm_sbindings_apply with these explicit arguments. RU: Вызвать fvm_sbindings_apply с этими явными аргументами. */
```

[Complete calling context](../../examples/bridge/main.c#L83)

## See also

- [Implementation: protocol/sbindings.c:348](../../protocol/sbindings.c#L348)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
