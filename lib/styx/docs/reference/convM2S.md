# convM2S

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](convM2S.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

uint convM2S(uchar *ap, uint nap, Fcall *f);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `ap` | `uchar *ap` | Mutable input message; decoded string/data pointers borrow this storage. |
| `nap` | `uint nap` | Accessible input bytes, including the four-byte message length. |
| `f` | `Fcall *f` | Output Fcall storage. On zero return, discard its potentially partial fields. |

## Description

Decode one original 9P2000 message, validating counted fields inside its declared extent.

ap is mutable message storage of nap bytes. Counted strings are shifted in place to add NUL terminators. Fcall strings/data borrow ap: retain that storage until processing finishes. A zero return means incomplete/invalid input; discard the decoded object rather than trusting partial fields. Success is the declared message extent, not sizeof(Fcall).

### Preserved implementation/header contract

no syntactic checks.
three causes for error:
 1. message size field is incorrect
 2. input buffer too short for its own data (counts too long, etc.)
 3. too many names or qids
gqid() and gstring() return nil if they would reach beyond buffer.
main switch statement checks range and also can fall through
to test at end of routine.

## Return value

`uint`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
check(nreply > 0 && convM2S(reply, nreply, &out) == nreply, "decode");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L182)

## See also

- [Implementation: vendor/inferno/lib9/convM2S.c:105](../../vendor/inferno/lib9/convM2S.c#L105)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
