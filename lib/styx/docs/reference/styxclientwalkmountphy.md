# styxclientwalkmountphy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwalkmountphy.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxClient *styxclientwalkmountphy(RemotePhy *phy,
                                   unsigned msize,
                                   const char *user,
                                   const char *name,
                                   unsigned timeout,
                                   unsigned limit,
                                   unsigned long tag);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `msize` | `unsigned msize` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `user` | `const char *user` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `name` | `const char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |
| `timeout` | `unsigned timeout` | Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract. |
| `limit` | `unsigned limit` | Explicit local quota; permitted ranges and wire/body units are given in the description. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |

## Description

styxclientwalkmountphy: native client traversal operations and lifetime.

### Preserved implementation/header contract

EN/RU: Explicit negotiated S-record history profile around unchanged Styx frames; consumes PHY always.
Both endpoints must enable the profile. tag is a nonzero direction-local assignment in tag0 controls.
No raw fallback/dial/authentication; active history must have exportable VM identities.
Явный S-record history profile, PHY принят всегда; обе стороны включают profile, tag согласуется через tag0.

## Return value

`StyxClient *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: client/traversal.c:35](../../client/traversal.c#L35)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
