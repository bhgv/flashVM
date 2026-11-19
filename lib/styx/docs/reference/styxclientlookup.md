# styxclientlookup

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientlookup.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxFid *styxclientlookup(StyxClient *client, const char *path, StyxCancel *cancel, int *missing);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `path` | `const char *path` | Selected object path or stable identity according to the declared type; no implicit authority is granted. |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |
| `missing` | `int *missing` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientlookup: native client walk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Root-relative lookup with per-call copied absence, owned fid/NULL; missing required.
Borrow client/path/cancel through return. Missing1 only exact acknowledged Inferno
Enonexist under explicit profile; all other failures leave missing0. Inferno lookup
walks one component per RPC so a partial Rwalk cannot erase the failure reason.
Native PHY or legacy supported; non-NULL cancel requires PHY owner. No data retry.
Root-relative lookup, свой fid/NULL, required missing;1 только explicit Inferno
Enonexist, прочие ошибки0. Inputs borrowed, по одному компоненту под Inferno profile.

## Return value

`StyxFid *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxFid *styxclientlookup(StyxClient *client, const char *path, StyxCancel *cancel, int *missing);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L268)

## See also

- [Implementation: client/walk.c:366](../../client/walk.c#L366)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
