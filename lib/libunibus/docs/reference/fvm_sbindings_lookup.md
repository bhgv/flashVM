# fvm_sbindings_lookup

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sbindings_lookup.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_sbindings_lookup(const FVM_SBindings *table, unsigned long tag, FVM_SBinding *out);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `table` | `const FVM_SBindings *table` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `out` | `FVM_SBinding *out` | Caller-owned protocol value/state; native layout is never a wire representation. |

## Description

fvm_sbindings_lookup: direction-local numeric tag to schema identity bindings.

### Preserved implementation/header contract

Copy an association to caller output. UNKNOWN for zero/unbound tags; outputs unchanged on
error. Lookup copies metadata, not a VM/native object; no allocation or implicit permission
grant.

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

- [Implementation: protocol/sbindings.c:247](../../protocol/sbindings.c#L247)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
