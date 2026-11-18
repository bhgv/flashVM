/* Copyright (c) 2026 bhgv
 * EN: Private terminal queues, owned fids and native cancellation; no managed pointers.
 * RU: Private terminal queues, owned fids и native cancellation без managed pointers.
 */
#ifndef STYX_CONTROL_INTERNAL_H
#define STYX_CONTROL_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include "control.h"
#include "cancel.h"
#include "flashvm/sstream.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: Bounded fixed-slot ring protected by control mutex; no runtime allocation.
 * Bounded ring под control mutex, без runtime allocation. */
typedef struct ControlQueue
{
    unsigned char *bytes; /* Owned slots times extent storage. */
    size_t sizes[32]; /* Payload/encoded extents by slot. */
    unsigned long tags[32]; /* RX tags only, no semantic interpretation. */
    unsigned first; /* First occupied slot. */
    unsigned count; /* Occupied slot count. */
} ControlQueue;


/* EN/RU: Immutable storage plus mutex-protected queues/status; worker-local offsets/decoder.
 * Immutable storage и queues/status под mutex; offsets/decoder worker-local. */
struct StyxBidirControl
{
    pthread_mutex_t lock; /* Protect queues, reason and active cancellation pins. */
    pthread_cond_t changed; /* Monotonic native capacity/output notifications. */
    pthread_t workers[2]; /* One RX and one TX native I/O owner. */
    unsigned started; /* Successfully created workers for setup rollback. */
    StyxClient *client; /* Owned extra session pin. */
    StyxFid *rx; /* Worker-owned read endpoint, cleanup after join. */
    StyxFid *tx; /* Worker-owned write endpoint, cleanup after join. */
    StyxCancel *active[2]; /* Borrowed worker-owned tokens under mutex, never reused. */
    unsigned limit; /* Body quota including encoded tag. */
    unsigned slots; /* Fixed slots per direction. */
    size_t extent; /* Limit plus conservative S-record framing reserve. */
    int reason; /* Sticky first terminal reason. */
    DVM_Cancel *retired; /* Owned sticky native notification source; callbacks run outside control lock. */
    ControlQueue incoming; /* Verified individual records, drainable after stop/failure. */
    ControlQueue outgoing; /* Encoded frames; head remains pinned until acknowledged. */
    unsigned char *decode; /* RX-only incremental decoder storage. */
};


/* EN/RU: Publish first terminal reason and signal active RPC tokens under lock; notify native taps after unlock.
 * Первый reason и active RPC tokens под lock, native taps после unlock, без guest calls. */
void controlend(StyxBidirControl *control, int reason);


/* EN/RU: Execute one cancellable native read/write; borrowed buffers/result, worker number0 RX/1 TX.
 * Return0 if already terminal,1 after RPC; token published before admission and retired after RPC.
 * Один native RPC, worker0 RX/1 TX;0 terminal/1 completed, token published до admission. */
int controlio(StyxBidirControl *control, unsigned worker, unsigned long long offset,
    void *bytes, size_t size, StyxTransfer *result);


/* EN/RU: Native RX/TX entries; borrowed control retained through join. / Native workers, control до join. */
void *controlreceive(void *control);


/* EN/RU: Native TX entry; borrowed control, no foreign callbacks. / Native TX, без foreign callbacks. */
void *controltransmit(void *control);
#endif
