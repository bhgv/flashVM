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
#include "packet.h"
#include <stdint.h>
#include <time.h>
#include <errno.h>


/* EN/RU: Per-connection logical fid state only; original server independently owns all authoritative fid/Qid storage.
 * Только logical fid state соединения; original server независимо владеет authoritative fid/Qid storage.
 */
typedef struct StyxWalkFid
{
    u32int id; /* Original protocol fid number, not a native pointer. */
    int directory; /* Last acknowledged Qid kind. */
    uvlong node; /* Acknowledged held-object Qid path; names alone do not identify an opened object after replacement. */
    int opened; /* Topen acknowledged; Tstat then describes the held object rather than named metadata. */
    char path[WALK_PATH_LIMIT]; /* Logical location within the fixed exported root, used only for traversal identity. */
    struct StyxWalkFid *next; /* Owned bounded metadata chain. */
} StyxWalkFid;


/* EN/RU: Owner-only I/O state; only references and immutable PHY wake access cross threads.
 * Owner-only I/O state; между threads только references и immutable PHY wake access. */
struct StyxPhyWake
{
    int walk_mode; /* Explicit negotiated history envelope, never inferred from ordinary traffic. */
    unsigned long walk_tag; /* Data purpose assigned by the peer in tag0, acknowledged on every reply. */
    WalkEntity walk_root; /* Trusted fixed granted-root identity, not a host path or credential. */
    int walk_cycle; /* Explicit current callback failure kind; unrelated skipped cycles never annotate denials. */
    unsigned long walk_before; /* Native cycle serial at this original-core dispatch entry. */
    StyxWalkFid *walk_fids; /* Owned scalar active-fid metadata, bounded separately from original server authority. */
    StyxWalkFid *walk_pending; /* Owned staged attach/walk candidate until acknowledged reply. */
    unsigned walk_fid_count; /* Actual scalar tracker count. */
    unsigned char *walk_decode; /* Owned separate original-request codec scratch in history mode. */
    Fcall walk_request; /* Borrowed decoded pointers into walk_decode through this one dispatch. */
    char walk_base[WALK_PATH_LIMIT]; /* Copied original walk base, stable for in-place/partial acknowledgement. */
    char walk_target[WALK_PATH_LIMIT]; /* Immutable active endpoint remainder through dispatch. */
    unsigned capacity; /* Bounded incoming encoded-frame storage. */
    unsigned stride; /* Fixed outgoing slot extent, reply history is always empty. */
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

/* EN/RU: Shared explicit raw/history constructor; consumes PHY always, copies optional root metadata. */
char *styxphyinitprofile(Styxserver *, Styxops *, RemotePhy *, int, int, unsigned, const WalkEntity *);

/* EN/RU: Import a complete control/data request before raw dispatch,1/0; caller leaves successful scope. */
int styxphywalkenter(StyxPhyWake *, WalkScope *);

/* EN/RU: Refuse a proven repeated endpoint without executing original side effects;1/0 enqueue result. */
int styxphywalkreject(Styxserver *);

/* EN/RU: Original backend reply enqueue; borrowed bytes/count, return accepted raw extent/-1. */
int styxsend(Styxserver *, int, char *, int, int);

/* EN/RU: Restore endpoint request scope, apply only acknowledged fid metadata, and free tracker owners. */
int styxphywalkrequest(StyxPhyWake *, const void *, unsigned, WalkScope *);
int styxphywalkresponse(StyxPhyWake *, const void *, unsigned);
void styxphywalkclear(StyxPhyWake *);
void styxphyoutput(StyxPhyWake *context);


/* EN/RU: Native monotonic milliseconds, negative on clock failure; no carrier or guest calls.
 * Native monotonic milliseconds, negative при clock failure; без carrier/guest calls. */
double styxphytime(void);
#endif
