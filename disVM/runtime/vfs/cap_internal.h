/* Copyright (c) 2026 bhgv
 * [GC:] EN: Exact managed capability shells and native-only publication state.
 * RU: Exact managed capability shells и native-only publication state.
 */
#ifndef DVM_VFS_CAP_INTERNAL_H
#define DVM_VFS_CAP_INTERNAL_H
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "kernel.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "shared_pointer.h"
#include "export.h"
#include "guest.h"
#include "close_worker.h"
#include "channel_sync.h"
#include "vfsexport_decl.h"
#include <stdlib.h>
#include <string.h>
#define DVM_CAP_ROOT 0U /* Immutable tree capability. */
#define DVM_CAP_OWNER 1U /* Publication administrative authority. */
#define DVM_CAP_ENDPOINT 2U /* Request submission only. */
#define DVM_CAP_TICKET 3U /* Requester result/cancellation authority. */
#define DVM_CAP_REQUEST 4U /* Immutable owner-side review capability. */
#define DVM_CAP_TYPES 5U /* Exact runtime type identities. */


/* EN/RU: Service lifetime is shared by owner, endpoints and review cards.
 * Native service lifetime разделён owner/endpoints/review cards. */
typedef struct DVM_CapService
{
    FVM_PortSync *lock; /* Serializes requests/review/close; no managed work under lock. */
    FVM_PortSync *lifetime; /* Short refcount lock, never held by source callbacks or SDK operations. */
    size_t references; /* All native capability states retaining this service. */
    DVM_Publication *publication; /* Owned publisher, NULL after explicit close. */
    DVM_Provider *events; /* Owned ordinary change source, independent of service lifetime. */
} DVM_CapService;


/* EN/RU: Immutable pointer from managed shell; all mutable state is native-locked.
 * Immutable pointer managed shell; mutable state защищено native lock. */
typedef struct DVM_CapState
{
    FVM_PortSync *lock; /* Serializes ticket result transfer/cancellation. */
    unsigned kind; /* Exact type expected by the managed shell. */
    DVM_Provider *root; /* Owned tree or cached granted result. */
    DVM_CapService *service; /* Retained publication service for owner/endpoint/request. */
    DVM_ExportTicket *ticket; /* Owned native requester capability, NULL after cancellation. */
    DVM_ExportRequest request; /* Copied immutable owner-side review card. */
    int status; /* Cached requester Pending/Granted/Denied/Consumed state. */
} DVM_CapState;


/* [GC:] EN/RU: Allocate exact rooted shell and native cleanup reservation; nil on native OOM.
 * Создать exact rooted shell и cleanup reservation; nil при native OOM. */
void *dvm_cap_new(unsigned kind, DVM_CapState **state);


/* [GC:] EN/RU: Publish and unroot newly owned shell, preserving displaced owners on refusal.
 * Опубликовать и снять private root; displaced owner сохраняется до guarded replacement. */
void dvm_cap_publish(void *value, void *destination);


/* [GC:] EN/RU: Drop staged rooted shell on refusal/error; no inline native destruction.
 * Снять staged rooted shell при отказе/error; без inline native destruction. */
void dvm_cap_abandon(void *value);


/* [GC:] EN/RU: Check exact type; nil returns NULL, forged records raise exHeap.
 * Проверить exact type; nil даёт NULL, forged record вызывает exHeap. */
DVM_CapState *dvm_cap_get(void *value, unsigned kind);


/* EN/RU: Validate/copy borrowed guest string before native release;1/0.
 * Проверить/copy guest string до native release; без retained guest pointers. */
int dvm_cap_text(String *value, char *out, size_t capacity);


/* EN/RU: Retain/drop service under native lifetime lock; final drop closes publication.
 * Удержать/снять service; final drop закрывает publication, no managed callbacks. */
int dvm_cap_service_retain(DVM_CapService *service);


/* EN/RU: Consume service ref; final release closes native publication and change source.
 * Поглотить service ref; final release закрывает publication/change source. */
void dvm_cap_service_drop(DVM_CapService *service);


/* EN/RU: Create ordinary notification node and native notify callback.
 * Создать ordinary notification node и native callback; owned provider/NULL. */
DVM_Provider *dvm_cap_events(void);


/* EN/RU: Under service operation serialization, push a pending-request invalidation.
 * Под service operation serialization выдать invalidation о pending request. */
void dvm_cap_notify(void *context);
#endif
