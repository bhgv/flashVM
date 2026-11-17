/* Copyright (c) 2026 bhgv
 * EN: Native server transport staging, original-core hooks and independently retained wake context.
 * RU: Native server transport staging, original-core hooks и independently retained wake context.
 */
#ifndef STYX_PHY_INTERNAL_H
#define STYX_PHY_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include <stdint.h>
#include <time.h>
#include <errno.h>


/* EN/RU: Owner-only I/O state; only references and immutable PHY wake access cross threads.
 * Owner-only I/O state; между threads только references и immutable PHY wake access. */
struct StyxPhyWake
{
    RemotePhy *phy; /* Owned until the final owner/wake pin retires. */
    size_t references; /* Atomic server owner plus independent native wake pins. */
    uchar *receive; /* Owned one-frame receive buffer, MSGMAX bytes. */
    unsigned used; /* Received confirmed prefix. */
    unsigned wanted; /*4 for size header, then full original frame length. */
    int complete; /* One full frame may be handed to original server. */
    uchar *output; /* Owned bounded ring of queued_frames*MSGMAX bytes. */
    unsigned *lengths; /* Owned per-slot encoded extents. */
    unsigned slots; /* Fixed reply queue bound1..128. */
    unsigned first; /* Current reply slot. */
    unsigned queued; /* Queued complete reply slots. */
    unsigned sent; /* Confirmed prefix of current first slot. */
    int accepted; /* Single already connected peer admitted exactly once. */
    int processing; /* Reserve one reply slot while original core may generate a response. */
    int failed; /* Sticky carrier/framing/queue failure. */
};
extern __thread StyxPhyWake *styxphyowner; /* Current original-core call on this native owner only. */


/* EN/RU: Original core entry points renamed only in the inclusion translation unit.
 * Исходные entry points переименованы только в inclusion translation unit. */
char *styxcoreinit(Styxserver *, Styxops *, char *, int, int);
char *styxcoreprocess(Styxserver *);
void styxendserver(Styxserver *);


/* EN/RU: Owner pumps bounded nonblocking receive/send; callbacks never enter a VM.
 * Owner выполняет bounded nonblocking receive/send; callbacks без VM entry. */
void styxphyreceive(StyxPhyWake *context);
void styxphyoutput(StyxPhyWake *context);


/* EN/RU: Native monotonic milliseconds, negative on clock failure; no carrier or guest calls.
 * Native monotonic milliseconds, negative при clock failure; без carrier/guest calls. */
double styxphytime(void);
#endif
