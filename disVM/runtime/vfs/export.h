/* Copyright (c) 2026 bhgv
 * EN: Explicit export-tree composition and capability-based asynchronous publication requests.
 * RU: Явная композиция export tree и capability-based asynchronous publication requests.
 */
#ifndef DVM_VFS_EXPORT_H
#define DVM_VFS_EXPORT_H
#include "stack.h"
#define DVM_VIEW_TREE 1U /* Delegate descendants; absent means exactly one published node. */
#define DVM_EXPORT_PREPARE 1U /* Publication option: invoke owner preparation or queue for deferred review. */
#define DVM_EXPORT_REMOTE 1U /* Trusted transport submitted a remote request. */
#define DVM_EXPORT_AUTHENTICATED 2U /* Trusted transport attests peer identity; never accept this bit from wire input. */
#define DVM_EXPORT_PEER_BYTES 128U /* Copied authenticated peer label, not an authority token. */
#define DVM_EXPORT_LIMIT 256U /* Maximum outstanding native tickets per publication. */
typedef struct DVM_View DVM_View; /* Mutable builder, consumed by freeze on every outcome. */
typedef struct DVM_Publication DVM_Publication; /* Owner capability; no global registry. */
typedef struct ExportEndpoint ExportEndpoint; /* Submit-only capability; pins closed publisher storage. */
typedef struct DVM_ExportTicket DVM_ExportTicket; /* Requester's native capability, not a guessable integer. */


/* EN/RU: Return1 to permit current virtual operation; access0 is metadata visibility.
 * Native-only bounded local decisions, no managed callbacks/namespace reentry.
 * Provider I/O belongs to source callbacks. Context lives until release.
 * Разрешить virtual operation:1; access0 — metadata. Context живёт до release. */
typedef int (*DVM_ViewPolicy)(void *context, const char *path, unsigned access);


/* EN/RU: Create owned empty/default-deny builder, taking context only on success.
 * Создать owned empty/default-deny builder, context принять только при успехе. */
DVM_View *dvm_view_create(DVM_ViewPolicy policy, void *context, void (*release)(void *));


/* EN/RU: Add/replace exact virtual mapping; NULL source hides its prefix. Paths must
 * be canonical, absolute virtual and relative source names (dot for root). Retains
 * source on success; access limits every operation; TREE delegates descendants.
 * Добавить mapping, NULL скрывает prefix; source удержать при успехе. Return1/0. */
int dvm_view_map(DVM_View *view, const char *path, DVM_Provider *source, const char *relative, unsigned access, unsigned flags);


/* EN/RU: Freeze immutable mappings into owned provider, consuming builder on every
 * outcome. Stable-bind promise requires all mapped sources stable. No source I/O.
 * Зафиксировать mappings в provider, builder поглотить всегда; NULL on error. */
DVM_Provider *dvm_view_freeze(DVM_View *view);


/* EN/RU: Consume unused builder and retained sources/context, NULL allowed.
 * Поглотить builder, источники и context; NULL допустим. */
void dvm_view_discard(DVM_View *view);


/* EN/RU: Capture native directory identity or parent/file capability from borrowed
 * native plan. Owned provider/NULL; native I/O outside admission/namespace locks.
 * Захватить native directory либо parent/file по plan; owned provider/NULL. */
DVM_Provider *dvm_export_native(DVM_ResourcePlan *plan, int directory);


/* EN/RU: Mount and bind an owned-provider capability into current chosen group with
 * the same indexed-stack semantics. A file may target an absent leaf in an existing directory.
 * Provider remains borrowed; 1/0/-1 success/stale/error.
 * Смонтировать и bind provider capability по indexed-stack rules; provider borrowed. */
int dvm_stack_mount(DVM_Namespace *group, DVM_Provider *provider, const char *target, unsigned flags, int64_t index);


/* EN/RU: Copied request, logical name is never resolved as a host filesystem path.
 * Copied request; logical name никогда автоматически не открывает host path. */
typedef struct DVM_ExportRequest
{
    uint64_t id; /* Publisher-local review identity, not a redemption capability. */
    unsigned flags; /* Remote/authenticated provenance from trusted transport. */
    unsigned kind; /* Requested hint:0 unspecified,1 file,2 directory; never an authority grant. */
    unsigned access; /* Requested access ceiling, narrowed again at grant. */
    char peer[DVM_EXPORT_PEER_BYTES]; /* Trusted authenticated identity or local caller label. */
    char name[DVM_NS_PATH_LIMIT]; /* Logical publication request, not a source path. */
} DVM_ExportRequest;


/* EN/RU: Notify owner after queue publication, outside mutex; enqueue native work
 * only, never execute/pause a guest. Borrowed context outlives submitters/close.
 * Уведомить owner вне mutex; только native work, без guest execution/pause. */
typedef void (*DVM_ExportNotify)(void *context);


/* EN/RU: Owner preparation callback:1 returns an owned chosen provider,0 defers
 * to next/reply,-1 denies. Requested/returned trees may differ. Native callbacks
 * only; a Limbo adapter must queue work to its owner instead of executing foreign VM.
 * Подготовка:1/owned provider,0/defer,-1/deny; дерево может отличаться от запроса. */
typedef int (*DVM_ExportPrepare)(void *context, const DVM_ExportRequest *request, DVM_Provider **chosen);


/* EN/RU: Optional admission policy; direct mode requires this explicit gate.
 * Опциональная admission policy; direct mode требует explicit gate.1 allow,0 deny. */
typedef int (*DVM_ExportAuthorize)(void *context, const DVM_ExportRequest *request);


/* EN/RU: Copied publication settings; callbacks/context borrowed through joined close.
 * Copied settings; callbacks/context живут до joined close. */
typedef struct DVM_ExportOptions
{
    size_t limit; /* Outstanding ticket bound1..256. */
    unsigned flags; /* PREPARE or0; never Inferno export flags. */
    DVM_Provider *direct; /* Retained prepublished ordinary tree for direct mode. */
    DVM_ExportAuthorize authorize; /* Request admission gate, mandatory in direct mode. */
    DVM_ExportPrepare prepare; /* Optional native preparation callback, PREPARE mode only. */
    DVM_ExportNotify notify; /* Native notification when a deferred request becomes readable. */
    void *context; /* Borrowed policy/notification state, no managed pointers. */
} DVM_ExportOptions;


/* EN/RU: Create owned publication, retaining direct tree; NULL on invalid/OOM.
 * Создать publication, удержать direct tree; NULL invalid/OOM. */
DVM_Publication *dvm_publication_create(const DVM_ExportOptions *options);


/* EN/RU: Queue/process copied request; id is assigned internally. Remote requires
 * authenticated provenance from trusted adapter, not a client-supplied wire bit.
 * Returns owned opaque ticket/NULL. No implicit host path lookup or target execution.
 * Принять copied request; opaque ticket/NULL, без host path lookup/guest execution. */
DVM_ExportTicket *dvm_publication_request(DVM_Publication *publication, const DVM_ExportRequest *request);


/* EN/RU: Create submit-only endpoint from a live borrowed owner; owned result/NULL.
 * Serialize creation against owner close. No reply/next/close authority is exported.
 * Создать submit-only endpoint; свой результат/NULL, создание до owner close. */
ExportEndpoint *exportendpoint(DVM_Publication *publication);


/* EN/RU: Submit through borrowed endpoint; owned ticket/NULL, same admission rules.
 * Endpoint can outlive joined owner close; closed publishers refuse without callbacks.
 * Запросить через endpoint; свой ticket/NULL, после close отказ без callbacks. */
DVM_ExportTicket *exportrequest(ExportEndpoint *endpoint, const DVM_ExportRequest *request);


/* EN/RU: Consume endpoint after its callers retire; NULL allowed. No owner close.
 * Поглотить endpoint после завершения вызовов; NULL допустим, owner не закрывать. */
void exportendpointfree(ExportEndpoint *endpoint);


/* EN/RU: Take exactly one pending request for owner review;1/0, copied output.
 * Получить ровно один pending request для owner review;1/0. Native nonwaiting. */
int dvm_publication_next(DVM_Publication *publication, DVM_ExportRequest *output);


/* EN/RU: Owner explicitly grants a chosen prepared provider or refuses with NULL.
 * Grant is additionally restricted to requested rights.1 accepted,0 gone/answered/closed.
 * Owner выдаёт выбранный provider либо NULL/deny;1/0, без source callback под mutex. */
int dvm_publication_reply(DVM_Publication *publication, uint64_t id, DVM_Provider *chosen);


/* EN/RU: Nonwaiting ticket result:0 pending,1 owned grant,2 denied,-1 consumed/invalid.
 * Takes grant once; requester must serialize ticket calls and release the ticket.
 * Читать result:0 pending,1 owned grant,2 deny,-1 invalid; grant выдать один раз. */
int dvm_export_take(DVM_ExportTicket *ticket, DVM_Provider **grant);


/* EN/RU: Cancel/drop requester capability and unlink request; late reply refuses.
 * Отменить/drop ticket; late reply отказывает. No managed access, NULL allowed. */
void dvm_export_release(DVM_ExportTicket *ticket);


/* EN/RU: Close publisher, deny unanswered requests, consume owner reference. Stop
 * and join active submitter/owner calls before close; idle endpoints, tickets and
 * grants can outlive it. Endpoint calls after close safely refuse; free callback
 * context only after active calls retire.
 * Закрыть publisher, deny pending, поглотить owner ref; tickets/grants живут дальше. */
void dvm_publication_close(DVM_Publication *publication);
#endif
