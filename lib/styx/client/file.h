/* Copyright (c) 2026 bhgv
 * EN: Scalar C boundary between Inferno callbacks and owned VFS node capabilities.
 * RU: Скалярная C-граница callbacks Inferno и владеющих VFS node-capabilities.
 */
#ifndef STYX_FILE_H
#define STYX_FILE_H
#include <stddef.h>
#define STYX_NAME_MAX 4096U             /* Complete UTF-8 component capacity, including NUL. */
typedef struct StyxBacking StyxBacking; /* Owned prepared node capability. */
typedef struct StyxOpen StyxOpen;       /* One exclusive open, never shared between fids. */

typedef struct StyxMeta
{
    unsigned long long identity, device, length; /* Full-width native identity and length. */
    unsigned long mode;                          /* Display permissions, never an authority grant. */
    int directory;                               /* Opened directories use tree.h cursor APIs. */
    char name[STYX_NAME_MAX];                    /* Complete displayed basename, never a host path. */
} StyxMeta;

typedef struct StyxTransfer
{
    size_t count;                            /* Confirmed prefix only. */
    int again;                               /* Live source temporarily empty; native server may defer, never EOF. */
    int cancelled;                           /* Native cancellation/timeout without original acknowledgement. */
    int success, confirmed, uncertain, stop; /* Original native outcome, no automatic retry. */
} StyxTransfer;

#endif
