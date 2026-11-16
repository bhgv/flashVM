/* Copyright (c) 2026 bhgv
 * EN: Private bounded fd-owner transactions, original/flush tag leases and native state.
 * RU: Private bounded fd-owner transactions, original/flush tag leases и native state.
 */
#ifndef STYX_MUX_INTERNAL_H
#define STYX_MUX_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include <lib9.h>
#include <styx.h>
#include "mux.h"
#include <pthread.h>
#include <poll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>


/* EN/RU: Caller waits natively; engine owns admitted flight until done/removal.
 * Caller ждёт native; engine владеет flight до done/removal. */
struct StyxFlight
{
    StyxMux *mux; /* Borrowed engine, pinned by caller client lifetime. */
    StyxCancel *cancel; /* Retained until caller collection. */
    pthread_cond_t condition; /* One waiting native RPC caller. */
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
    int flushing; /* Cancellation/deadline requested; do not restart grace. */
    int original; /* Complete validated original reply is preserved. */
    int done; /* Removed from table; caller may collect/free. */
    StyxAnswer answer; /* Owned raw response and precise transaction outcome. */
};


/* EN/RU: One autonomous native owner, no VM/client pointers or foreign callbacks.
 * Один autonomous native owner, без VM/client pointers и foreign callbacks. */
struct StyxMux
{
    pthread_mutex_t lock; /* Protect admission, table, completion and references. */
    pthread_cond_t ended; /* Explicit native shutdown completion. */
    int fd; /* Consumed connected stream, closed by native owner. */
    int wake[2]; /* Owned nonblocking notification pipe until final ref. */
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
    int finished; /* Socket closed; thread reference retired. */
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
/* EN/RU: Detached native fd owner; argument is its retained engine reference.
 * Detached native fd owner; argument — удержанная engine reference. */
void *styxmuxrun(void *);
#endif
