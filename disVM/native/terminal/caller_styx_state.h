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

typedef struct CallerIo CallerIo; /* Dispatcher-owned deferred byte transfer. */


/* EN: Keep the exclusive file open until the fid and its transfer retire.
 * RU: Удерживать открытый файл до завершения fid и его передачи данных. */
typedef struct CallerOpen
{
    StyxOpen *file;       /* Exclusive opened VFS cursor. */
    uint64_t generation;  /* Publication granting the open, zero for the ordinary view. */
    CallerIo *active;    /* Active transfer using this cursor, on the dispatcher. */
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
    CallerIo *pending; /* Owned deferred transfers, retired only on this dispatcher. */
    unsigned io_count; /* Bounded count including cancelling transfers. */
};


/* EN: Release an owned open on the dispatcher. Its active transfer can delay cleanup.
 * RU: Освободить открытие у диспетчера. Активная передача может отложить очистку. */
void styxcaller_close_open(void *state);


/* EN: Defer file I/O on server at offset. Copy count input bytes for writes,
 * or pass NULL input for reads. Borrow opened through completion. Set count
 * to zero and return nil on admission, otherwise return a static Styx error.
 * RU: Отложить I/O на server со смещения offset. Скопировать count входных
 * байтов для записи или передать NULL input для чтения. Удерживать opened
 * до завершения. При успехе обнулить count и вернуть nil, иначе ошибку Styx. */
char *styxcaller_defer_io(StyxCallerServer *server, CallerOpen *opened, const void *input,
    ulong *count, vlong offset);


/* EN: Publish finished transfers. When closing, join all after styxend removes
 * the wire requests. Other calls collect only completed transfers.
 * RU: Выдать завершённые передачи у диспетчера. При закрытии дождаться всех
 * после снятия запросов через styxend. Иначе собрать лишь завершённые операции. */
void styxcaller_finish_io(StyxCallerServer *server, int closing);

#endif
