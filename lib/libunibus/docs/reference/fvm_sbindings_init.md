# fvm_sbindings_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sbindings_init.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_sbindings_init(FVM_SBindings *table, FVM_SBinding *slots, size_t capacity);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `table` | `FVM_SBindings *table` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `slots` | `FVM_SBinding *slots` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |

## Description

fvm_sbindings_init: direction-local numeric tag to schema identity bindings.

### Preserved implementation/header contract

Initialize an empty table without modifying slots; caller synchronizes all shared use.
Capacity zero/null slots is valid. Reinitializing invalidates prior session associations.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
fvm_sbindings_init(&bindings, slots, 2) == FVM_S_OK;
```

[Complete calling context](../../examples/bridge/main.c#L177)

## See also

- [Implementation: protocol/sbindings.c:217](../../protocol/sbindings.c#L217)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
