/* Copyright (c) 2026 bhgv
 * EN: Private Inferno-side client state; never include VFS/VM SDK type definitions.
 * RU: Private Inferno-side client state; без VFS/VM SDK type definitions.
 */
#ifndef STYX_CLIENT_INTERNAL_H
#define STYX_CLIENT_INTERNAL_H
#include <lib9.h>
#include <styx.h>
#include <pthread.h>
#include "client.h"
#include "mux.h"
#include "life.h"

/* EN/RU: One connection owns codecs/buffers and serializes bounded native RPCs.
 * Одно соединение владеет buffers и сериализует bounded native RPCs. */
struct StyxClient
{
    StyxLife *life; /* Owned sticky terminal source; no provider/guest pointers. */
    int terminal; /* First legacy terminal reason, serialized by client lock. */
    StyxMux *mux; /* Optional independent native owner; no Client pointers retained by it. */
    int transportclosed; /* Legacy transport close already consumed by explicit shutdown. */
    int cancelled; /* Current returned RPC cancelled, under client lock. */
    pthread_mutex_t lock; /* Protects refs/fids/reply copying; legacy RPC stays serialized. */
    StyxTransport transport; /* Legacy adopted stream; unused with native fd owner. */
    uchar *send; /* Owned bounded outgoing frame. */
    uchar *receive; /* Owned bounded incoming frame, decoded pointers borrow it. */
    unsigned msize; /* Negotiated complete-message bound. */
    u32int lastfid; /* Monotonic remote fid allocator; never wraps/reuses. */
    size_t references; /* Owner plus separately owned fids. */
    Qid root; /* Attached root identity/type. */
    int failed; /* Fatal framing/transport failure; no automatic reconnection. */
    int attempted; /* Last RPC attempted stream write; errors may have effects. */
};


/* EN/RU: One independently owned remote object retains its client and data ceiling.
 * Один independent remote object удерживает client и data ceiling. */
struct StyxFid
{
    StyxClient *client; /* Retained connection until clunk. */
    u32int number; /* Independent remote fid, never keyed by Qid. */
    Qid qid; /* Last walked/opened remote identity. */
    unsigned unit; /* Maximum read/write payload, negotiated and Ropen-limited. */
    int opened; /* Topen succeeded. */
    int mode; /* Fixed read/write ceiling0/1/2. */
    int poisoned; /* Unknown open/data effects forbid later open/data calls. */
};


/* EN/RU: Under client lock, exchange one frame;1 expected reply,0 Rerror,-1 fatal.
 * Returned pointers borrow receive until the next RPC. No retry of failed effects.
 * Под lock обменять frame;1 reply,0 Rerror,-1 fatal; pointers заимствованы. */
int styxrpc(StyxClient *client, Fcall *request, Fcall *reply);
int styxrpccancel(StyxClient *, Fcall *, Fcall *, StyxCancel *);
/* EN/RU: Borrow client under lock for untimed native read; cancel/terminal still retires it.
 * Под client lock выполнить untimed read; cancel/terminal завершают его. */
int styxrpcwatch(StyxClient *, Fcall *, Fcall *, StyxCancel *);
void styxrpcbad(StyxClient *); /* Client lock held; terminal failure wakes all multiplexed work. */
StyxClient *styxclientstart(const StyxTransport *, StyxMux *, unsigned, const char *, const char *);


/* EN/RU: Borrow pinned client under its lock; publish first legacy reason outside that lock.
 * Temporarily unlocks/relocks client; failed exchange has no usable borrowed reply.
 * Под client lock опубликовать первый legacy reason вне lock; client удержан caller. */
void styxclientemit(StyxClient *client, int reason);


/* EN/RU: Under client lock, consume remote fid when the stream remains usable.
 * Под lock поглотить remote fid, если stream ещё пригоден. */
void styxclunknumber(StyxClient *client, u32int number);


/* EN/RU: Clear borrowed outcome before validation; count remains zero for control RPCs.
 * Очистить borrowed outcome до validation; count ноль для control RPC. */
void styxopclear(StyxTransfer *result);


/* EN/RU: Copy latest RPC outcome while client lock is held; status1/0/-1.
 * Unacknowledged sent requests are uncertain, never inferred successful or replayed.
 * Копировать latest outcome под client lock; sent без acknowledgement — uncertain. */
void styxopresult(StyxClient *client, int status, StyxTransfer *result);


/* EN/RU: Schedule best-effort clunk of an abandoned number; client lock held.
 * Worker owns only client/number. Setup failure fails the connection without RPC wait.
 * Отложить best-effort clunk abandoned number; setup failure завершает connection. */
void styxforgetlocked(StyxClient *client, u32int number);

#endif
