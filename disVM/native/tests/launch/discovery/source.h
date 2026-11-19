/* Copyright (c) 2026 bhgv
 * EN: Native advertised/captured source fixtures and independent live view policy.
 * RU: Native advertised/captured source fixtures и independent live view policy.
 */
#ifndef LAUNCH_DISCOVERY_SOURCE_H
#define LAUNCH_DISCOVERY_SOURCE_H
#include "../backend.h"
#include "../../../../runtime/vfs/export.h"
#include "../../../../runtime/vfs/launch/route.h"
#define SOURCE_FLAGS (DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE | DVM_PROVIDER_CANCEL_LAUNCH) /* Full fixture opt-in. */


/* EN/RU: Counters outlive every source/guard, updates are native atomic scalars only.
 * Counters живут дольше sources/guards, updates — только native atomic scalars.
 */
typedef struct SourceStats
{
    unsigned created; /* Successful native source factories. */
    unsigned destroyed; /* Actual final source destructor calls. */
    DVM_Cancel *cancel_on_query; /* Optional borrowed per-operation source, set only by the synchronous cancellation fixture. */
    unsigned queries; /* Actual launch callback calls, excluding generic preflight refusals. */
} SourceStats;


/* EN/RU: Caller-owned live policy state, retained by fixture until all views/addresses retire.
 * Caller-owned live policy state до завершения всех views/addresses.
 */
typedef struct Policy
{
    int allowed; /* Atomic execute permission for this exported view. */
    unsigned released; /* Actual final view-context release count. */
} Policy;


/* EN/RU: New owned source/NULL; clone borrowed endpoint and copy prefix, flags are explicit native promises.
 * Prefix is canonical relative node/dot; publish callback only when advertise1. All caller inputs stay borrowed.
 * Новый owned source/NULL; clone endpoint, copy prefix, flags — explicit promises; inputs borrowed.
 */
DVM_Provider *source_create(DVM_LaunchEndpoint *endpoint, SourceStats *stats, const char *prefix,
    unsigned flags, int advertise);


/* EN/RU: Live view decision; state/path/access borrowed,1/0, no provider I/O or guest access.
 * Live view decision; state/path/access borrowed,1/0, без provider I/O/guest access.
 */
int policy_check(void *state, const char *path, unsigned access);


/* EN/RU: Record final context retirement; caller keeps policy storage alive through this callback.
 * Учесть final context retirement; caller удерживает policy storage до callback.
 */
void policy_release(void *state);


/* EN/RU: Receive/free one real owner request and requester ticket, requiring the source's owner-side module label.
 * Получить/освободить real owner request и ticket, проверить module label источника.
 */
void source_received(DVM_LaunchService *owner, DVM_LaunchTicket *ticket);
#endif
