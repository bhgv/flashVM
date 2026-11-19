# styxphywalkinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphywalkinit.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

char *styxphywalkinit(Styxserver *server,
                      Styxops *ops,
                      RemotePhy *phy,
                      int permissions,
                      int needfile,
                      unsigned queued_frames,
                      const WalkEntity *root);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `ops` | `Styxops *ops` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `permissions` | `int permissions` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `needfile` | `int needfile` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `queued_frames` | `unsigned queued_frames` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `root` | `const WalkEntity *root` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxphywalkinit: phy/traversal implementation and native state.

### Preserved implementation/header contract

EN/RU: Explicit S-record active-history profile around original Styx; PHY consumed always, root copied on success.
Both peers opt in; root describes the virtual exported endpoint of the actual prepared grant and must have nonzero domain/object.
Tag0 negotiates data purpose; malformed profiles close without raw fallback. Same owner/quota rules as styxphyinit.
Явный S-record history profile; PHY принят всегда, root — virtual exported endpoint actual grant с nonzero domain/object.
Обе стороны opt in; tag0 согласует purpose, malformed profiles закрываются без raw fallback.

## Return value

`char *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: server/phy/traversal.c:70](../../server/phy/traversal.c#L70)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
