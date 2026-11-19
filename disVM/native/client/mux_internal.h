/* Copyright (c) 2026 bhgv
 * EN: Private bounded carrier-owner transactions, original/flush tag leases and native state.
 * RU: Private bounded carrier-owner transactions, original/flush tag leases и native state.
 */
#ifndef STYX_MUX_INTERNAL_H
#define STYX_MUX_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include "internal.h"
#include "mux.h"
#include "life.h"
#include "packet.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>


/* EN/RU: Caller waits natively; engine owns admitted flight until done/removal.
 * Caller ждёт native; engine владеет flight до done/removal. */
struct StyxFlight
{
    StyxMux *mux; /* Borrowed engine, pinned by caller client lifetime. */
    StyxCancel *cancel; /* Retained until caller collection. */
    pthread_cond_t condition; /* One waiting native RPC caller. */
    uchar *wire; /* Optional owned S-record envelope, captured on original caller before broker admission. */
    uchar *flushwire; /* Optional owned encoded flush envelope, separate from original request. */
    unsigned flushlength; /* Actual flush wire extent, raw profile remains nine bytes. */
    uchar *request; /* Owned encoded original frame. */
    unsigned length; /* Complete original frame size. */
    unsigned sent; /* Original frame bytes accepted by send. */
    unsigned bound; /* Negotiated response bound for this request. */
    unsigned slot; /* Reserved engine table slot. */
    ushort tag; /* Original tag, NOTAG only for version. */
    ushort flush; /* Separate paired tag, leased through Rflush. */
    uchar flushbytes[9]; /* Original-codec Tflush frame. */
    unsigned flushsent; /* Tflush frame bytes sent. */
    double deadline; /* Absolute original deadline in monotonic milliseconds. */
    double flushdeadline; /* One nonrenewable cancellation grace deadline. */
    int watching; /* Long-lived read has no ordinary RPC deadline; cancellation grace remains bounded. */
    int flushing; /* Cancellation/deadline requested; do not restart grace. */
    int original; /* Complete validated original reply is preserved. */
    int done; /* Removed from table; caller may collect/free. */
    StyxAnswer answer; /* Owned raw response and precise transaction outcome. */
};


/* EN/RU: One bounded no-fid probe; both tags are permanently outside RPC leases.
 * Один bounded probe без fid; оба tag постоянно вне RPC leases. */
typedef struct StyxBeat
{
    unsigned long long completed; /* Saturating count of actually acknowledged probes, under mux lock. */
    uchar *wire; /* Optional owned negotiated envelope for this exact heartbeat probe. */
    unsigned length; /* Actual wire extent, nine bytes in the raw profile. */
    unsigned idle; /* Future idle interval, zero disables new probes. */
    unsigned reply; /* Future probe window in monotonic milliseconds. */
    unsigned misses; /* Future terminal threshold2..16. */
    unsigned budget; /* Captured current window, immutable after admission. */
    unsigned limit; /* Captured current threshold. */
    unsigned sent; /* Bytes of current nine-byte probe accepted by send. */
    unsigned missed; /* Already observed elapsed windows. */
    uchar bytes[9]; /* Original-codec Tflush to unused oldtag. */
    double due; /* Next idle probe time. */
    double deadline; /* Send/response phase deadline; partial progress never renews it. */
    int active; /* Current tags retained until response or terminal close. */
    int notice; /* State transition to publish after engine unlock. */
    int suspect; /* An observation requires a recovery notification. */
} StyxBeat;


/* EN/RU: One autonomous native owner, no VM/client pointers or foreign callbacks.
 * Один autonomous native owner, без VM/client pointers и foreign callbacks. */
struct StyxMux
{
    unsigned long walk_tag; /* Explicit direction-local traversal profile, zero means raw 9P. */
    int walk_bound; /* Server root assignment has been validated by a reply. */
    WalkEntity walk_root; /* Acknowledged immutable granted root, no source/VM pointers. */
    unsigned wire_capacity; /* Receive capacity includes only bounded reply envelope overhead. */
    StyxBeat beat; /* Native heartbeat policy and fixed probe storage. */
    StyxLife *life; /* Owned terminal source, independent of Client lifetime. */
    int localstop; /* Explicit shutdown reports CLOSED; normal owner drop stays quiet. */
    int terminal; /* Immutable first terminal cause. */
    pthread_mutex_t lock; /* Protect admission, table, completion and references. */
    pthread_cond_t ended; /* Native shutdown/probe acknowledgement predicate notifications. */
    RemotePhy *phy; /* Owned carrier; only native owner performs I/O, other pinned threads may wake. */
    unsigned capacity; /* Maximum initial offered message bytes. */
    unsigned limit; /* Bounded outstanding request slots1..128. */
    unsigned timeout; /* Original and one flush grace budget in milliseconds. */
    unsigned next; /* Round-robin original-frame selection. */
    unsigned used; /* Buffered input bytes. */
    unsigned want; /* Current header/full-frame receive target. */
    uchar *receive; /* Owned raw frame buffer. */
    uchar *decode; /* Owned original-codec scratch, never caller storage. */
    StyxFlight **slots; /* Owned bounded table of admitted flights. */
    StyxFlight *sending; /* Borrowed current noninterleaved output frame. */
    int sendflush; /* Current output is Tflush rather than original request. */
    size_t references; /* Client owner plus detached native thread. */
    int failed; /* Terminal transport/protocol failure. */
    int stopping; /* Explicit shutdown/final owner-drop request. */
    int finished; /* Carrier disconnected; notification fanout/thread reference may still be live. */
};


/* EN/RU: Native monotonic time, negative on clock failure; no callbacks.
 * Native monotonic time, отрицательное при ошибке; без callbacks. */
double styxmuxtime(void);
/* EN/RU: Under engine lock, retire one flight while preserving any known original reply.
 * Под engine lock убрать flight, сохранив known original reply. */
void styxmuxfinish(StyxMux *, StyxFlight *);
/* EN/RU: Under engine lock, fail and wake all admitted native callers.
 * Под engine lock отказать и разбудить всех admitted native callers. */
void styxmuxbroken(StyxMux *);
/* EN/RU: Release engine only after owner and native thread references retire.
 * Освободить engine после owner/native-thread refs. */
void styxmuxdestroy(StyxMux *);
/* EN/RU: Detached native carrier owner; argument is its retained engine reference.
 * Detached native carrier owner; argument — удержанная engine reference. */
void *styxmuxrun(void *);

/* EN/RU: Validate negotiated S-record reply under owner lock;1/0, cycle copied for original caller. */
int styxmuxwalkreply(StyxMux *mux, unsigned *cycle);


/* EN/RU: Under engine lock, heartbeat timer timeout/-1; expired policy breaks engine.
 * Под engine lock heartbeat timer timeout/-1; истёкшая policy завершает engine. */
int styxbeattick(StyxMux *, double);


/* EN/RU: Under engine lock, send pending probe without interleaving frames.
 * Под engine lock отправить probe без interleaving frames. */
void styxbeatsend(StyxMux *);


/* EN/RU: Under engine lock, validate leased probe response;1 valid,0 fatal,-1 ordinary tag.
 * Под engine lock проверить probe response;1 valid,0 fatal,-1 ordinary tag. */
int styxbeatreply(StyxMux *);


/* EN/RU: Under engine lock, valid response refreshes idle/reply windows but never releases a probe lease.
 * Под engine lock response обновляет idle/reply windows, не освобождая probe lease. */
void styxbeattraffic(StyxMux *);


/* EN/RU: Under engine lock, publish pending native observation by temporarily releasing it.
 * Под engine lock опубликовать observation с временным освобождением lock. */
void styxbeatreport(StyxMux *);
#endif
