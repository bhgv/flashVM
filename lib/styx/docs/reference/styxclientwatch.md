# styxclientwatch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwatch.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *));
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `notify` | `StyxTerminal notify` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `state` | `void *state` | Opaque callback context; its lifetime/transfer is specified by the creating API. |
| `void (*release)(void *)` | `void (*release)(void *)` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientwatch: native client session operations and lifetime.

### Preserved implementation/header contract

EN/RU: Adopt state only on success, owned watch/NULL; terminal state replays once.
At most256 live watcher nodes/client. Retain client while registering. State
outlives already admitted callbacks; release runs after final unsubscribe/flight.
Принять state только при успехе; свой watch/NULL, sticky terminal replay.

## Return value

`StyxWatch *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *));
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L115)

## See also

- [Implementation: client/readable_parts/session-c/part-002.inc:173](../../client/readable_parts/session-c/part-002.inc#L173)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
