/* Copyright (c) 2026 bhgv
 * EN: Bounded native lease ownership; GC finalizers enqueue metadata only.
 * RU: Ограниченное native lease ownership; GC finalizers ставят лишь metadata.
 */
#ifndef DVM_PROXY_GC_QUEUE_H
#define DVM_PROXY_GC_QUEUE_H
#include "../remote/session.h"
#include <pthread.h>

#define DVM_PROXY_LEASE_LIMIT 256U /* Bound active, staged and pending image slots. */


/* EN/RU: Private native ownership phases; these are not wire or managed IDs.
 * Приватные native ownership phases; это не wire/managed IDs. */
typedef enum
{
    DVM_PROXY_LEASE_FREE,    /* Slot reusable with a new serial. */
    DVM_PROXY_LEASE_STAGED,  /* Allocated before remote Retain. */
    DVM_PROXY_LEASE_ACTIVE,  /* Owns one remote reference. */
    DVM_PROXY_LEASE_PENDING, /* Finalized token awaits broker Release. */
    DVM_PROXY_LEASE_RELEASED /* Explicit Release consumed; token still exists. */
} DvmProxyLeaseState;


/* EN/RU: One native slot belongs to one managed lease until finalization/pop.
 * Один native slot принадлежит managed lease до finalization/pop. */
typedef struct
{
    uint64_t serial;          /* Never-reused incarnation within this queue. */
    DvmRemoteHandle owner;    /* Scalars only; no managed pointer or root. */
    DvmProxyLeaseState state; /* FREE/STAGED/ACTIVE/PENDING/RELEASED, private enum. */
} DvmProxyLeaseEntry;


/* EN/RU: Native lifetime is held by connection payload and every managed token.
 * Native lifetime удерживают connection payload и каждый managed token. */
typedef struct
{
    pthread_mutex_t mutex;                             /* Protects all queue metadata. */
    pthread_t thread;                                  /* Creating broker, never a migrating Prog. */
    size_t references;                                 /* Native strong owners, under mutex. */
    size_t limit;                                      /* Configured bound, at most LEASE_LIMIT. */
    size_t live;                                       /* Occupied slots, including pending release. */
    uint64_t next;                                     /* Zero means exhausted, never wraps. */
    int closed;                                        /* Terminal revocation of all images. */
    DvmProxyLeaseEntry entries[DVM_PROXY_LEASE_LIMIT]; /* No allocation in finalizers. */
} DvmProxyLeaseQueue;


/* EN/RU: Create one queue for this broker; NULL on invalid limit/OOM.
 * Создать очередь этого broker; NULL при неверном limit/OOM. */
DvmProxyLeaseQueue *dvm_proxy_lease_queue_new(size_t limit);


/* EN/RU: Terminal close discards pending entries, never performs VM/network work.
 * Terminal close удаляет pending entries без VM/network work. */
void dvm_proxy_lease_queue_close(DvmProxyLeaseQueue *queue);


/* EN/RU: Drop exactly one independently held native owner; destroy after last one.
 * Снять один native owner; уничтожить после последнего. */
void dvm_proxy_lease_queue_drop(DvmProxyLeaseQueue *queue);


/* EN/RU: Broker reserves a slot/native owner; zero on quota/closure/exhaustion.
 * Broker резервирует slot/native owner; ноль при quota/closure/exhaustion. */
uint64_t dvm_proxy_lease_new(DvmProxyLeaseQueue *queue, DvmRemoteHandle owner, int active, size_t *slot);


/* EN/RU: Read active state or broker-activate/consume once; action 0/1/2 respectively.
 * Читать active либо broker-activate/consume один раз; action 0/1/2 соответственно. */
int dvm_proxy_lease_action(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial, int action);


/* EN/RU: Collector finalizes one token: enqueue active release, otherwise free slot.
 * GC завершает token: active release ставится в очередь, иначе slot освобождается. */
void dvm_proxy_lease_retire(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial);


/* EN/RU: Broker pops one pending handle; output unchanged if none/closed/wrong thread.
 * Broker снимает pending handle; output неизменен при none/closed/wrong thread. */
int dvm_proxy_lease_pop(DvmProxyLeaseQueue *queue, DvmRemoteHandle *owner);


/* EN/RU: Joined diagnostics of native queue/token ownership; stderr only.
 * Joined diagnostics native queue/token ownership; только stderr. */
void dvm_proxy_lease_statistics(void);
#endif
