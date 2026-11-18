/* Copyright (c) 2026 bhgv
 * EN: ACK-gated native directional remapping on one ordered connection; codecs remain explicit host adapters.
 * RU: Native directional remapping через ACK одного ordered connection; codecs задаёт явный host adapter.
 */
#ifndef STYX_SCOPE_OPERATION_H
#define STYX_SCOPE_OPERATION_H
#include "scope.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_SCOPE_OPERATION_IDLE 0     /* No accepted proposal. */
#define STYX_SCOPE_OPERATION_WAITING 1  /* Only selected outgoing tags held pending matching peer confirmation. */
#define STYX_SCOPE_OPERATION_ACCEPTED 2 /* Matching ACK committed candidate TX tags. */
#define STYX_SCOPE_OPERATION_REJECTED 3 /* Explicit matching NACK preserved previous TX tags. */
#define STYX_SCOPE_OPERATION_TERMINAL 4 /* Broker retired; no uncertain proposal may resume TX. */


/* EN/RU: Native-only copy candidate sibling TX map and request body, hold selected outgoing tags, queue root0 request.
 * 1 admitted/0 refused, no I/O/wait; operation is nonzero strictly increasing local correlation ID per parent.
 * Independent RX/other TX subtrees remain live. No new wire ID/format: native adapter encodes operation/map.
 * Request follows already accepted frames; no automatic retry. Parent/list/body borrowed through return.
 * Native-only proposal TX map/request, 1/0; лишь affected TX tags ожидают ACK, RX/остальные TX продолжаются.
 */
int styxscopetxprepare(StyxScope *parent, unsigned long long operation,
    const StyxScopeAssignment *list, size_t count, const void *request, size_t size);


/* EN/RU: Native service adapter applies exact matching ACK(accepted1)/NACK(0), 1 applied/repeat, 0 stale/invalid.
 * ACK atomically commits copied candidate; NACK keeps prior map. Exact most-recent completed repeats are idempotent;
 * mismatched IDs/results never release a hold. Host authenticates/parses peer confirmation on this connection.
 * Native matching ACK/NACK, 1/0; ACK commit candidate, NACK прежняя карта; stale не снимает hold.
 */
int styxscopetxconfirm(StyxScope *parent, unsigned long long operation, int accepted);


/* EN/RU: Native-only atomic RX map commit plus copied root0 ACK admission; 1 committed/0 unchanged.
 * ACK storage allocated before mutation; successful reply never precedes RX commit. No codec/replay cache inferred.
 * Adapter validates request identity, authorization and duplicate-request outcome before calling this function.
 * Native RX commit + root0 ACK atomарно, 1/0; storage до mutation, identity/access/replay решает adapter.
 */
int styxscoperxcommit(StyxScope *parent, const StyxScopeAssignment *list, size_t count, const void *reply, size_t size);


/* EN/RU: Coherent native operation state above, copied ID only on valid parent; invalid returns TERMINAL unchanged.
 * No queue consumption or transport access, parent pinned through return.
 * Coherent operation state, copied ID лишь valid parent, без queue consumption/I/O.
 */
int styxscopetxstate(StyxScope *parent, unsigned long long *operation);


/* EN/RU: Block native owner for this exact operation until resolution/retirement under one budget0..120000ms; 1 resolved/0 otherwise.
 * Timeout does not cancel/unfreeze or replay; inspect state. Unknown peer effects require reconciliation or root retirement.
 * Native owner ждёт resolution, 1/0; timeout не отменяет, не снимает hold и не повторяет request.
 */
int styxscopetxwait(StyxScope *parent, unsigned long long operation, unsigned timeout);
#ifdef __cplusplus
}
#endif
#endif
