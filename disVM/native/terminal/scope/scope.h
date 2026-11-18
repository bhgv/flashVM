/* Copyright (c) 2026 bhgv
 * EN: Explicit nested S-record broker with native namespace authority and isolated client inbox handles.
 * RU: Явный broker вложенных S-записей с native namespace authority и изолированными client inbox handles.
 */
#ifndef STYX_SCOPE_H
#define STYX_SCOPE_H
#include "control.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_SCOPE_RX 0U /* Incoming outer bindings, independent of outbound authority. */
#define STYX_SCOPE_TX 1U /* Explicit outbound envelope tags, never learned from RX. */
#define STYX_SCOPE_DEPTH 32U /* Local maximum envelope nesting, not a wire field. */
typedef struct StyxScope StyxScope; /* Native host namespace authority, never exported to an application. */
typedef struct StyxScopeClient StyxScopeClient; /* Application inbox/send capability, no transport service access. */


/* EN/RU: One native sibling assignment; tag0 unbinds, expected guards stale native control decisions.
 * Local generation is not a wire field; host must agree remote activation before remapping live traffic.
 * Native sibling assignment: tag0 отвязывает, expected защищает от stale control decisions, не wire field.
 */
typedef struct StyxScopeAssignment
{
    StyxScope *scope;            /* Direct child of the parent passed to assign. */
    unsigned long tag;           /* Nonzero Uint32 outer tag, zero unbinds. */
    unsigned long long expected; /* Exact current local generation for this direction. */
} StyxScopeAssignment;


/* EN/RU: Consume owned control on every outcome, return native root/NULL; start sole envelope broker.
 * Explicit profile: each envelope payload is a complete sequence of child S-records; leaves are opaque.
 * Host must create/bind recipients before peer data publication. Existing flat routes are not upgraded.
 * Принять control всегда, свой root/NULL; явный профиль полных child S-record sequences, leaves opaque.
 */
StyxScope *styxbidirscopes(StyxBidirControl *control, unsigned limit);


/* EN/RU: Native-only append a direct child group, root-owned until rootfree; NULL on allocation/depth/terminal refusal.
 * Borrow pinned parent; no tag assignment or application capability is created.
 * Native-only direct child group принадлежит root до free; NULL при отказе, parent pinned, без tag/client grant.
 */
StyxScope *styxscopegroup(StyxScope *parent);


/* EN/RU: Native-only create direct client child and its distinct inbox handle; NULL leaves output unchanged.
 * Parent/root pinned through return and all later client calls. / Создать client child и отдельный inbox,
 * NULL сохраняет output; parent/root удерживаются до завершения calls.
 */
StyxScope *styxscopeclient(StyxScope *parent, StyxScopeClient **client);


/* EN/RU: Atomically assign sibling tags for RX/TX, 1/0; zero count is valid, no partial changes.
 * Borrow native parent/list; reject stale generations, duplicate targets/final nonzero sibling tags,
 * foreign/non-child scopes and overflow. Accepted messages/encoded TX never consult later mappings.
 * Атомарные RX/TX sibling tags, 1/0; stale/duplicates/foreign/overflow отвергаются без partial changes.
 */
int styxscopeassign(StyxScope *parent, unsigned direction, const StyxScopeAssignment *assignments, size_t count);


/* EN/RU: Native-only copied tag/generation, 1/0, outputs unchanged on refusal; root has no parent binding.
 * Native-only copied tag/generation, 1/0, root без parent binding, invalid не меняет outputs.
 */
int styxscopebinding(StyxScope *scope, unsigned direction, unsigned long *tag, unsigned long long *generation);


/* EN/RU: Copy one client-local record, including its own tag0; same RECORD/EMPTY/END/SMALL contract.
 * One consumer per client; root owner remains pinned, client cannot access transport/group service queues.
 * Выдать одну client-local record, включая собственный tag0; client не видит transport/group services.
 */
int styxscopeclientnext(StyxScopeClient *client, unsigned long *tag, void *bytes, size_t capacity, size_t *size);


/* EN/RU: Block only this client for local readiness within monotonic timeout0..120000ms; no consumption.
 * Блокировать только клиента до local readiness, timeout0..120000ms, без consumption.
 */
int styxscopeclientwait(StyxScopeClient *client, unsigned timeout);


/* EN/RU: Native host reads one tag0 from a root/group, no application delivery; SMALL retains head.
 * Borrow scope/output, one native service consumer per group, no implicit command execution.
 * Native host читает один tag0 root/group; не application delivery, SMALL сохраняет head.
 */
int styxscopeservicenext(StyxScope *scope, void *bytes, size_t capacity, size_t *size);


/* EN/RU: Native service consumer blocks only for this root/group inbox, 1 ready/0 timeout or terminal.
 * Scope pinned through return, timeout0..120000ms, no consumption. / Native service consumer ждёт лишь
 * свой root/group inbox, 1/0, timeout0..120000ms, scope pinned, без consumption.
 */
int styxscopeservicewait(StyxScope *scope, unsigned timeout);


/* EN/RU: Client sends one local S-record, wraps every explicitly granted TX parent tag, 1 accepted/0 refused.
 * Copy before acceptance, no raw outer-tag access or RX-derived TX authority. Nonblocking TX admission.
 * Клиент передаёт local S-record в явно выданных TX envelopes, 1/0; без raw outer tags и RX->TX grants.
 */
int styxscopeclientsend(StyxScopeClient *client, unsigned long tag, const void *bytes, size_t size);


/* EN/RU: Native-only sticky root/group status; NULL returns READ_FAILED, no I/O or queue consumption.
 * Native-only sticky status, NULL возвращает READ_FAILED, без I/O/consumption.
 */
int styxscopestate(StyxScope *scope);


/* EN/RU: Native-only root stop; wakes independent inbox waits, preserves accepted records, no join.
 * Native-only root stop, будит inbox waits, сохраняет accepted records, без join.
 */
void styxscopestop(StyxScope *root);


/* EN/RU: Consume root after all native/client users retire; stop/join broker then owned control, NULL harmless.
 * Child handles die with root; never call from owned workers/retirement callbacks. Shared client stays hosted.
 * Поглотить root после native/client users; stop/join broker/control, child handles становятся invalid.
 */
void styxscopefree(StyxScope *root);
#ifdef __cplusplus
}
#endif
#endif
