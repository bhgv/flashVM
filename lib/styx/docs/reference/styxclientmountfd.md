# styxclientmountfd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientmountfd.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxClient *
styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fd` | `int fd` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `msize` | `unsigned msize` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `user` | `const char *user` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `name` | `const char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |
| `timeout` | `unsigned timeout` | Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract. |
| `limit` | `unsigned limit` | Explicit local quota; permitted ranges and wire/body units are given in the description. |

## Description

styxclientmountfd: native client fd operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume connected POSIX stream fd always; independent native owner supports
up to limit1..128 outstanding RPCs. timeout1..120000ms per request, then at most
one extra timeout interval for flush. No transport authentication is implied.
Принять connected POSIX fd; отдельный native owner, bounded RPC/flush.

## Return value

`StyxClient *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxClient *client = styxclientmountfd(descriptor, 8192, "demo", "", 5000, 8);
```

[Complete calling context](../../examples/client.c#L55)

## See also

- [Implementation: client/fd.c:34](../../client/fd.c#L34)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
