/* Copyright (c) 2026 bhgv
 * EN: Private copied service output, independently pinned handlers and staged TX candidates.
 * RU: Частные copies service output, pinned handlers и staged TX candidates.
 */
#ifndef STYX_SCOPE_SERVICE_INTERNAL_H
#define STYX_SCOPE_SERVICE_INTERNAL_H
#include "service.h"
#include "operation/operation.h"


/* EN/RU: One native service record, owned by outbox until copied into control TX or final disposal.
 * Native service record принадлежит outbox до control TX copy либо disposal.
 */
typedef struct ScopeServiceMessage
{
    struct ScopeServiceMessage *next; /* Owned outbox continuation. */
    size_t size;                      /* Exact copied root0 body. */
    unsigned char bytes[];            /* No pointers/native layout on the wire. */
} ScopeServiceMessage;


/* EN/RU: Independently retained native handler context, refs protected by hub mutex.
 * Отдельно удержанный native handler context, refs под hub mutex.
 */
typedef struct ScopeServiceHandler
{
    size_t references;        /* Registered owner plus admitted broker invocation. */
    StyxScopeService receive; /* Nonwaiting native-only handler. */
    void *state;              /* Owned adopted native state, never a guest pointer. */
    void (*release)(void *);  /* Final disposal outside hub locks. */
} ScopeServiceHandler;


/* EN/RU: Copied local TX proposal, owned by parent until matching confirmation or root disposal.
 * Копия local TX proposal принадлежит parent до confirmation либо root disposal.
 */
typedef struct ScopeProposal
{
    unsigned long long operation; /* Exact local request correlation, encoded by explicit adapter. */
    size_t count;                 /* Copied sibling assignment count. */
    StyxScopeAssignment list[];   /* Native-only capability references pinned by root. */
} ScopeProposal;
struct ScopeHub; /* Private native owner. */


/* EN/RU: Allocate exact copied root0 message/NULL, payload borrowed; enforce tag+body limit before allocation.
 * Выделить root0 copy/NULL, payload borrowed, проверить tag+body limit до allocation.
 */
ScopeServiceMessage *scopeservicecopy(struct ScopeHub *hub, const void *bytes, size_t size);


/* EN/RU: Adopt preallocated message into outbox under hub lock; nonfailing after caller's preflight.
 * Принять preallocated message в outbox под hub lock, без fallible work после preflight.
 */
void scopeservicequeue(struct ScopeHub *hub, ScopeServiceMessage *message);


/* EN/RU: Native service TX owner, may block only its own control admission; never the broker or a client.
 * Native service TX owner ждёт лишь свой control admission, не broker/client.
 */
void *scopeservicerun(void *state);


/* EN/RU: Invoke pinned native hook outside locks for one root record, PASS/HANDLED/ERROR.
 * Вызвать pinned native hook вне locks для root record, PASS/HANDLED/ERROR.
 */
int scopeservicehandle(struct ScopeHub *hub, const void *bytes, size_t size);


/* EN/RU: Drop one handler ref under hub lock, return final context/NULL for release outside lock.
 * Снять handler ref под hub lock, final context/NULL освободить вне lock.
 */
ScopeServiceHandler *scopeservicedrop(ScopeServiceHandler *handler);


/* EN/RU: Consume final handler state outside locks; NULL harmless, no owner access permitted in release.
 * Освободить final handler вне locks; NULL безопасно, release не обращается к owner.
 */
void scopeservicefree(ScopeServiceHandler *handler);
#endif
