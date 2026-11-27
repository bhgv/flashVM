/* Copyright (c) 2026 bhgv
 * EN: Shared metadata for the movie slot and its named-channel reservations.
 * RU: Общие метаданные слота фильма и его резервов имён каналов.
 */
#ifndef STYXRPC_MODULE_SLOT_INTERNAL_H
#define STYXRPC_MODULE_SLOT_INTERNAL_H
#include "styxrpc/module_slot.h"
#include "nodes_internal.h"
#include <pthread.h>

typedef struct SlotFile SlotFile; /* Owner's tracking of a nested open fid. */
typedef struct SlotClaim SlotClaim; /* One private named-channel reservation. */

struct StyxRpcModuleSlot
{
    StyxRpcControl *control; /* Borrowed stable control tree context. */
    StyxRpcModule *module;   /* Borrowed image through retirement. */
    uint32_t generation;    /* Monotonic image identity. */
    StyxRpcNodes *nodes;     /* Owned active movie's callback nodes. */
    uint64_t next_node;      /* Monotonic generation-zero Qid within the session. */
    int retiring;            /* Reject changes during synchronous context release. */
    uint32_t revision;       /* Membership revision of the session root. */
    SlotFile *node_files;    /* Opens protected until callback withdrawal. */
    SlotFile *opened;        /* Fids for the current image. */
    StyxRpcEntry entry;      /* Borrowed enumeration result storage. */
    pthread_mutex_t names;  /* Protects claims and structural changes to nodes. */
    SlotClaim *claims;       /* Owned reservations, omitted from ordinary enumeration. */
    uint64_t last_claim;     /* Last issued reservation identity. */
};


/* EN: With the names lock held, check whether a callback path crosses a claim.
 * RU: Под блокировкой имён проверить, пересекается ли путь колбэка с резервом. */
int styxrpc_slot_claim_conflict(const StyxRpcModuleSlot *slot, const char *path);

/* EN: Reserve absolute path against files and channels in the slot, returning its exact claim.
 * RU: Зарезервировать абсолютный path для слота, вернув идентификатор резерва. */
uint64_t styxrpc_slot_claim(void *context, const char *path);

/* EN: Release the reservation identified by context, path and id.
 * RU: Освободить резерв точных context, path и id. */
void styxrpc_slot_drop(void *context, const char *path, uint64_t id);


/* EN: Release remaining reserved paths after the named dispatchers are gone.
 * RU: Освободить оставшиеся резервы после завершения именованных диспетчеров. */
void styxrpc_slot_claims_clear(StyxRpcModuleSlot *slot);
#endif
