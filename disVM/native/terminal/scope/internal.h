/* Copyright (c) 2026 bhgv
 * EN: One native nested broker, host-owned scope tree and per-client copied record FIFOs.
 * RU: Один native nested broker, host-owned scope tree и отдельные FIFO копий records клиентов.
 */
#ifndef STYX_SCOPE_INTERNAL_H
#define STYX_SCOPE_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include "scope.h"
#include "service/internal.h"
#include "cancel.h"
#include "flashvm/srecord.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: Copied accepted/staged message, never a native pointer on the wire.
 * Копия accepted/staged message, native pointers никогда не передаются по wire.
 */
typedef struct ScopeMessage
{
    struct ScopeMessage *next; /* Owned FIFO/staging continuation. */
    struct StyxScope *target;  /* Immutable resolved recipient during admission. */
    unsigned long tag;         /* Recipient-local tag; zero is local to this scope. */
    size_t size;               /* Exact copied body extent. */
    unsigned char bytes[];     /* Copied body, leaf payload never recursively guessed. */
} ScopeMessage;


/* EN/RU: Shared owner remains pinned through root disposal, all state protected by lock except worker buffer.
 * Общий owner живёт до root disposal; всё state под lock, кроме worker buffer.
 */
typedef struct ScopeHub
{
    pthread_mutex_t lock;              /* Tree, generations, committed queues and first reason. */
    pthread_cond_t changed;            /* Independent client/service readiness and retirement. */
    pthread_t sender;                  /* Dedicated native service outbox owner. */
    int sender_started;                /* Service worker must be joined before disposal. */
    ScopeServiceMessage *service_head; /* Owned copied root0 output queue. */
    ScopeServiceMessage *service_tail; /* Borrowed newest service output. */
    size_t service_count;              /* Includes head while native control admission is in progress. */
    ScopeServiceHandler *handler;      /* Owned registered native root handler. */
    pthread_t worker;                  /* Sole nested dispatch owner. */
    int started;                       /* Worker exists and must be joined. */
    int reason;                        /* First terminal outcome, queued records remain drainable. */
    unsigned char *scratch;            /* Owned broker-only input storage, allocated before publication. */
    unsigned limit;                    /* Maximum envelope/inner S-record body. */
    size_t queued;                     /* Total accepted unread records, checked for overflow. */
    StyxBidirControl *control;         /* Owned explicit native bidir control. */
    struct StyxScope *root;            /* Owned native tree root. */
    struct ScopeRetirement *notice;    /* Independent callback fence. */
    DVM_CancelTap *retired;            /* Owned control retirement subscription. */
} ScopeHub;


/* EN/RU: Native host scope authority; applications receive only the embedded inbox handle.
 * Native host scope authority; applications получают лишь embedded inbox handle.
 */
struct StyxScope
{
    ScopeHub *hub;                     /* Borrowed shared owner through rootfree. */
    struct StyxScope *parent;          /* Immutable parent, NULL for transport root. */
    struct StyxScope *children;        /* Owned direct children. */
    struct StyxScope *sibling;         /* Owned continuation in parent's child list. */
    unsigned depth;                    /* Root0, bounded by STYX_SCOPE_DEPTH. */
    int leaf;                          /* Client envelope parses messages once, group routes deeper. */
    unsigned long tags[2];             /* Independent RX/TX parent tags, zero unbound. */
    unsigned long long generations[2]; /* Local expected-generation guards, never serialized. */
    struct StyxScope *tx_pending;      /* This outer tag is reserved by an unresolved native proposal owner. */
    ScopeProposal *proposal;           /* Owned ACK-gated TX candidate, NULL when resolved. */
    size_t tx_holds;                   /* Selected pending tags at/below node protect this ancestor wrapper. */
    unsigned long long tx_last;        /* Greatest admitted local proposal ID, never reused. */
    unsigned long long tx_done;        /* Most recently resolved proposal ID. */
    int tx_accepted;                   /* Cached last ACK/NACK outcome for exact duplicate confirmation. */
    ScopeMessage *head;                /* Owned client records or native group tag0 notices. */
    ScopeMessage *tail;                /* Borrowed last committed FIFO node. */
    size_t count;                      /* Current unread recipient messages. */
    struct StyxScopeClient *client;    /* Owned client-only handle, NULL for groups. */
};


/* EN/RU: Opaque application capability; only its own queue and explicitly wrapped TX are public.
 * Opaque application capability; public лишь своя очередь и явно обёрнутый TX.
 */
struct StyxScopeClient
{
    StyxScope *scope; /* Borrowed native node, remains alive through root owner pin. */
};


/* EN/RU: Latch first hub reason, wake waits, then stop owned control outside hub lock.
 * Зафиксировать first reason и разбудить waits, затем stop control вне hub lock.
 */
void scopeend(ScopeHub *hub, int reason);


/* EN/RU: Native broker entry borrowing hub through join, no guest execution.
 * Native broker entry, hub borrowed до join, без guest execution.
 */
void *scoperun(void *state);


/* EN/RU: Feed one verified outer record atomically; hub pinned, 1 accepted/0 stopped or protocol/NOMEM.
 * Under hub lock captures bindings, validates whole nested envelope and stages copies before publication.
 * Принять один outer record атомарно, 1/0; bindings/validation/staging под hub lock, без partial publication.
 */
int scopefeed(ScopeHub *hub, unsigned long tag, const void *bytes, size_t size);


/* EN/RU: Attach/detach independent control-retirement fence, setup-only or joined-disposal-only.
 * Подписать/отделить independent retirement fence, лишь setup либо joined disposal.
 */
int scoperetirementattach(ScopeHub *hub);


/* EN/RU: Fence hub access then detach without waiting for late callbacks; only after broker/users retire.
 * Отделить hub access до nonwaiting detach, лишь после broker/users.
 */
void scoperetirementdetach(ScopeHub *hub);


/* EN/RU: Consume isolated tree after users/worker retire; recursion bounded by declared depth.
 * Поглотить isolated tree после users/worker, recursion ограничена depth.
 */
void scopeclear(StyxScope *scope);

/* EN/RU: Check/optionally commit assignments under hub lock, no allocation/locking or proposal-policy check.
 * Проверить/commit assignments под hub lock, без allocation/locking либо proposal-policy check.
 */
int scopeassignlocked(StyxScope *parent, unsigned direction, const StyxScopeAssignment *list, size_t count, int apply);

/* EN/RU: Under hub lock, check whether this tag/path may be changed while unrelated proposals stay live.
 * Под hub lock проверить mutable tag/path, не запрещая unrelated proposals.
 */
int scopetxmutable(StyxScope *scope);
#endif
