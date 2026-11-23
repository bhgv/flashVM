/* Copyright (c) 2026 bhgv
 * EN: Private bounded queues and synchronized native mux lifetime; no guest state.
 * RU: Закрытые bounded queues и synchronized native lifetime mux, без guest state.
 */
#ifndef PHY_MUX_INTERNAL_H
#define PHY_MUX_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include "bidir.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MUX_HEADER 12U /* P9MX v1 header bytes. */
#define MUX_PAYLOAD 1024U /* Maximum nonempty data payload. */


/* EN/RU: Mutex-protected circular byte queue; storage owned by hub. / Кольцевая очередь под mutex. */
typedef struct MuxQueue
{
    unsigned char *bytes; /* Owned bounded storage. */
    size_t first; /* First valid byte index. */
    size_t count; /* Valid bytes, never exceeds capacity. */
} MuxQueue;


/* EN/RU: One logical stream; mutable fields protected by hub lock. / Логический поток под hub lock. */
typedef struct MuxChannel
{
    struct PhyBidir *hub; /* Retained enclosing allocation through endpoint destruction. */
    unsigned id; /* Immutable wire channel. */
    int taken; /* Owner-only construction cannot repeat. */
    int closed; /* Local explicit abort, pending queue discarded. */
    int remote; /* Peer CLOSE observed after its preceding data. */
    int sentclose; /* CLOSE already selected by native pump. */
    int wake; /* Atomic sticky local caller interrupt. */
    unsigned generation; /* Atomic Linux futex publication counter, naturally aligned. */
    size_t credit; /* Peer-granted remaining transmit allowance. */
    size_t allowance; /* Locally granted remaining receive allowance. */
    size_t grant; /* Freed receive space awaiting a CREDIT frame. */
    MuxQueue rx; /* Confirmed incoming bytes. */
    MuxQueue tx; /* Locally accepted bytes, not peer delivery. */
} MuxChannel;


/* EN/RU: Single physical owner, independent logical owners under lock. / Один physical owner, logical owners под lock. */
struct PhyBidir
{
    RemotePhy *carrier; /* Immutable owned driver pointer, wake alive through final ref. */
    size_t capacity; /* Immutable per-queue byte bound. */
    size_t refs; /* Lock-protected hub plus endpoint lifetime refs. */
    pthread_mutex_t lock; /* Protect all mutable shared channel/hub state. */
    pthread_t worker; /* Joined exactly once by hub owner. */
    int stop; /* Owner requests native pump retirement. */
    int failed; /* Sticky transport/protocol termination. */
    unsigned turn; /* Pump-only round-robin next output channel. */
    MuxChannel channels[2]; /* Two prearranged independent streams. */
};


/* EN/RU: Queue helpers require hub lock, count <= available capacity; no allocation/failure.
 * Очереди под hub lock, count <= available capacity, без allocation/failure. */
void muxput(MuxQueue *queue, size_t capacity, const void *bytes, size_t count);


/* EN/RU: Remove count <= queue count into borrowed bytes under lock; no allocation.
 * Извлечь count <= queue count в borrowed bytes под lock, без allocation. */
void muxget(MuxQueue *queue, size_t capacity, void *bytes, size_t count);


/* EN/RU: Consume one ref under internal locking; final destroy after worker join and users retire.
 * Поглотить ref под lock; final destroy после worker join и retirement users. */
void muxdrop(PhyBidir *hub);


/* EN/RU: Native physical owner entry, borrowed hub held through join; no guest/callback execution.
 * Native physical owner, hub удержан до join, без guest callbacks. */
void *muxpump(void *state);


/* EN/RU: Construct logical endpoint over retained channel, owned/NULL; caller holds owner lifetime.
 * Создать logical endpoint над channel, свой/NULL, caller удерживает hub. */
RemotePhy *muxendpoint(MuxChannel *channel);

/* EN/RU: Nonwaiting generation publication for pinned channel, no producer mutex. / Generation wake без mutex. */
void muxsignalchannel(MuxChannel *channel);


/* EN/RU: Publish native readiness for both channels under retained hub lifetime. / Readiness для обоих каналов. */
void muxsignal(PhyBidir *hub);


/* EN/RU: Kernel wait if generation unchanged, borrowed relative monotonic timeout/NULL;0 normal/-1 error.
 * Kernel wait при прежнем generation, borrowed relative timeout/NULL;0 normal/-1 error. */
int muxsleep(MuxChannel *channel, unsigned generation, const struct timespec *timeout);
#endif
