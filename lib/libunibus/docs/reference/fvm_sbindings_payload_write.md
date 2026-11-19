# fvm_sbindings_payload_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sbindings_payload_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/sbindings.h>

FVM_SResult
fvm_sbindings_payload_write(const FVM_SBinding *bindings, size_t count, void *output, size_t capacity, size_t *used);
```

## Library

Unibus::wire; C89, no OS/allocator/threads

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `bindings` | `const FVM_SBinding *bindings` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `count` | `size_t count` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sbindings_payload_write: direction-local numeric tag to schema identity bindings.

### Preserved implementation/header contract

Encode only the tag-zero payload, then wrap it with fvm_srecord_write(CONTROL_TAG,...).
Payload = schema version, count, repeated(tag, schema version, identity length, identity
bytes). Validates the full list/capacity before writing; no duplicate/zero tags, no NUL/space
identities. Input associations/output must not overlap; used remains unchanged on failure.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::wire; C89, no OS/allocator/threads

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: protocol/sbindings.c:285](../../protocol/sbindings.c#L285)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
