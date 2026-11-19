# styxclientread

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientread.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientread(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `offset` | `unsigned long long offset` | Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning. |
| `bytes` | `void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `result` | `StyxTransfer *result` | Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect. |

## Description

styxclientread: native client io operations and lifetime.

### Preserved implementation/header contract

EN/RU: One actual Tread/Twrite chunk, signed64 offset range; exact confirmed
prefix, no auto retry. Uncertain data effects poison further data calls on fid.
Один Tread/Twrite chunk; точный prefix, без retry; uncertain effects запрещают I/O.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxclientread(fid, offset, bytes, sizeof(bytes), &result); /* EN: Invoke styxclientread with these explicit arguments. RU: Вызвать styxclientread с этими явными аргументами. */
```

[Complete calling context](../../examples/client.c#L67)

## See also

- [Implementation: client/io.c:116](../../client/io.c#L116)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
