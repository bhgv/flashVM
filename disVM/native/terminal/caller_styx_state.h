/* Copyright (c) 2026 bhgv
 * EN: Private ownership shared by the reverse Styx dispatcher and its controls.
 * RU: Общее внутреннее владение встречного диспетчера Styx и его управления.
 */
#ifndef STYX_CALLER_STATE_H
#define STYX_CALLER_STATE_H
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "caller_styx.h"
#include "caller_scope.h"
#include "server.h"
#include "tree.h"

#define PROG_QID 1U /* Permanent virtual directory for active process IDs. */

typedef struct CallerRead CallerRead; /* Dispatcher-owned deferred byte read. */


/* EN: Keep an exclusive file open until both the fid and any native read retire.
 * RU: Удерживать открытый файл до завершения fid и возможного нативного чтения. */
typedef struct CallerOpen
{
    StyxOpen *file;       /* Exclusive opened VFS cursor. */
    uint64_t generation;  /* Publication granting the open, zero for the ordinary view. */
    CallerRead *read;     /* Active provider read using the cursor, on the dispatcher. */
    int retired;         /* The dispatcher has released the original fid. */
} CallerOpen;


typedef struct CallerNodes
{
    uint64_t generation; /* Publication represented by this dispatcher cache. */
    size_t count; /* Number of visited nodes for this publication. */
    StyxBacking *nodes[STYX_CALLER_NODES]; /* Owned visited nodes. */
} CallerNodes;

struct StyxCallerServer
{
    Styxserver server; /* Owned original Styx dispatcher. */
    StyxPhyWake *wake; /* Owned dispatcher wake pin through mount retirement. */
    pthread_t worker; /* Owns dispatcher, per-PID nodes and stat scratch. */
    StyxCallerScopes scopes; /* Concurrent per-PID publications. */
    size_t references; /* Control and bound mount owners, under scopes.lock. */
    CallerNodes cache[STYX_CALLER_SLOTS]; /* Dispatcher-owned node caches. */
    StyxRemoveNode remove; /* Removes visible entries in the mutable shared view. */
    void *remove_state; /* Borrowed state for the remove callback. */
    DVM_Provider *shared; /* Retained optional view with its own access ceiling. */
    StyxBacking *shared_root; /* Identity-bound permanent root for ordinary walks. */
    const char *service_name; /* Borrowed exact root protocol component, or NULL. */
    size_t service_qid; /* Reserved permanent slot, valid with service_name. */
    CallerNodes permanent; /* Ordinary visited nodes, interned under scopes.lock. */
    int reserving_create; /* Dispatcher holds one permanent slot through provider create. */
    StyxMeta stat; /* Borrowed by Rstat encoding until callback completes. */
    CallerRead *reads; /* Owned deferred reads, retired only on this dispatcher. */
    unsigned read_count; /* Bounded count including cancelled reads still unwinding. */
};


/* EN: Release an owned open on the dispatcher. Its active read may delay freeing it.
 * RU: Освободить открытие у диспетчера. Активное чтение может отложить очистку. */
void styxcaller_close_open(void *state);


/* EN: Hold an eligible file read on server using the original deferred Tread support.
 * Take the capacity from count and set it to zero on success. Borrow opened through
 * completion. Return a static Styx error if admission fails.
 * RU: Отложить чтение открытого файла на server штатным механизмом Tread.
 * При успехе взять ёмкость из count и обнулить его. Удерживать opened до
 * завершения. При отказе вернуть статическую ошибку Styx. */
char *styxcaller_defer_read(StyxCallerServer *server, CallerOpen *opened, ulong *count, vlong offset);


/* EN: Publish finished reads on the dispatcher. When closing, join all reads after
 * styxend has removed the wire requests. Other calls reap only completed jobs.
 * RU: Выдать завершённые чтения у диспетчера. При закрытии дождаться всех чтений
 * после снятия запросов через styxend. Иначе собрать лишь завершённые операции. */
void styxcaller_finish_reads(StyxCallerServer *server, int closing);

#endif
