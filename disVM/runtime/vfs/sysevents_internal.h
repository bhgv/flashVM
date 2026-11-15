/* Copyright (c) 2026 bhgv
 * EN: Private native inbox fields, cached binding observers and owned message payload.
 * RU: Private native inbox fields, cached observers bindings и message payload.
 */
#ifndef DVM_VFS_SYSEVENTS_INTERNAL_H
#define DVM_VFS_SYSEVENTS_INTERNAL_H
#include "sysevents.h"
#include "owner_messages.h"
#include "channel_sync.h"
#define DVM_SYSEVENT_MESSAGE_BASE 65536U /* Separate native kind domain from owner request/reply controls. */


/* EN/RU: One current provider binding; all state is native and owned by inbox.
 * Один current provider binding; native state принадлежит inbox. */
typedef struct DVM_SysWatch
{
    struct DVM_SysWatch *next; /* Owned linked list. */
    DVM_Provider *provider; /* Retained factory, pins its journal. */
    DVM_SysEvents *events; /* Retained recipient, safe after detach until callback returns. */
    DVM_NoticeTap *tap; /* Owning source push subscription. */
    uint64_t epoch; /* Recipient namespace scope captured at subscription. */
    uint64_t last_session; /* Last directly delivered source epoch. */
    uint64_t last_generation; /* Last directly delivered report. */
    uint64_t identity; /* Current namespace binding identity, never a global provider ID. */
    char on[DVM_NS_PATH_LIMIT]; /* Virtual mount point only, never native source spelling. */
} DVM_SysWatch;


/* EN/RU: Generic owner-message prefix reuses the existing native mailbox engine.
 * Generic owner-message prefix использует существующий native mailbox engine. */
typedef struct DVM_SysMessage
{
    DVM_OwnerMessage message; /* Owned node, freed after consumption or conservative purge. */
    uint64_t session; /* Provider epoch, zero for namespace-wide events. */
    uint64_t epoch; /* Inbox namespace incarnation, fences staged FD records. */
    char on[1]; /* Variable-length copied virtual path; allocation includes final NUL. */
} DVM_SysMessage;


/* EN/RU: No Prog/heap pointers; owner and descriptors hold native references.
 * Без Prog/heap pointers; owner и FD удерживают native references. */
struct DVM_SysEvents
{
    FVM_PortSync *lock; /* Protects every mutable field and serializes native push and FD consumption. */
    size_t references; /* Owner, source taps and opened system-event descriptors. */
    DVM_Namespace *group; /* Retained current namespace, independent of Prog lifetime. */
    uint64_t version; /* Last observed group revision; UINT64_MAX forces first capture. */
    int captured; /* Initial snapshot exists even when revision is UINT64_MAX. */
    int resync; /* Allocation/overflow lost detail; next read must request full refresh. */
    uint64_t epoch; /* Monotonic inbox scope, invalidates staged records. */
    DVM_NoticeTap *namespace_tap; /* Owning namespace mutation push subscription. */
    DVM_SysWatch *watches; /* Cached automatic subscriptions for this group revision. */
    DVM_MessageRouter *router; /* Existing generic native mailbox implementation. */
    DVM_MessageEndpoint *endpoint; /* Private local ID1; not accessible by guest IDs. */
    size_t pending; /* Bounded queue length, maintained under lock. */
    int retired; /* No further source subscriptions/publication; FD can drain before EOF. */
};


/* EN/RU: Free detached watch list; no managed callbacks or namespace locks held.
 * Освободить detached watches; без managed callbacks и namespace locks. */
void dvm_sysevents_watches_free(DVM_SysWatch *watch);


/* EN/RU: Native mutation callback rebuilds subscriptions outside source locks.
 * Native mutation callback перестраивает subscriptions вне source locks. */
void dvm_sysevents_namespace(void *state, const struct DVM_ChangeEvent *event);


/* EN/RU: Retain current group/recipient into a namespace mutation subscription.
 * Удержать group/recipient в namespace subscription; caller retains group. */
DVM_NoticeTap *dvm_sysevents_subscribe_group(DVM_SysEvents *events, DVM_Namespace *group);


/* EN/RU: Under inbox lock, discard queued old-scope records. No callbacks.
 * Под inbox lock удалить queued old-scope records. Без callbacks. */
void dvm_sysevents_purge(DVM_SysEvents *events);


/* EN/RU: Under inbox lock, enqueue/coarsen a scalar/path notification.
 * Под inbox lock поставить/coarsen scalar/path notification. No remote I/O. */
int dvm_sysevents_post(DVM_SysEvents *events, unsigned kind, uint64_t source,
                       unsigned classes, uint64_t session, uint64_t generation, const char *on);


/* EN/RU: Open native readonly stream capturing retained inbox; empty output.
 * Открыть readonly stream с retained inbox; empty output. */
FVM_Result dvm_sysevents_open(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output);
#endif
