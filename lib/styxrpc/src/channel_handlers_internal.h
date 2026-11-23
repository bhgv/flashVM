/* Copyright (c) 2026 bhgv
 * EN: Shared state for the channel handler queue and its Styx file adapter.
 * RU: Общее состояние очереди обработчиков каналов и файлового адаптера Styx.
 */
#ifndef STYXRPC_CHANNEL_HANDLERS_INTERNAL_H
#define STYXRPC_CHANNEL_HANDLERS_INTERNAL_H
#include "styxrpc/channel_handlers.h"
#include "styxrpc/managed_module.h"
#include "module_nodes.h"
#include <pthread.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Byte capacity of each disjoint call buffer. */

typedef struct HandlerFile
{
    struct StyxRpcChannelHandlers *module;  /* Borrowed through close and completion. */
    const StyxRpcChannelFunction *function; /* Copied-result descriptor, if selected. */
    const StyxRpcChannelExport *channel;    /* Leased-result descriptor, if selected. */
    struct HandlerFile *next;               /* Queue link protected by module.lock. */
    uint64_t id;                           /* Exact opened node. */
    size_t staged;                         /* Request bytes accepted by the dispatcher. */
    int submitted;                         /* Sealed by the dispatcher after the last write. */
    int rejected;                          /* Sticky dispatcher-side framing failure. */
    int pending;                           /* Queued or executing, under module.lock. */
    int detached;                          /* FID closed while active, under module.lock. */
    size_t ready;                          /* Immutable reply length after completion. */
    int staged_output;                     /* Successful callback awaiting dispatcher publication. */
    int published;                         /* Exact token remains registered on the dispatcher. */
    StyxRpcChannelOutput output;           /* Resource transferred from the callback, if any. */
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Live token owned by issuing FID. */
    StyxRpcResource bus;                    /* Registry callbacks for an opened .unibus. */
    void *bus_cookie;                      /* Open registry operation handle. */
    StyxRpcChannelCall call;                /* Verified request whose payload borrows storage. */
    unsigned char storage[];               /* Request, reply and optional copied-result buffers. */
} HandlerFile;

struct StyxRpcChannelHandlers
{
    pthread_mutex_t lock;                   /* Protects ownership handoff and pending jobs. */
    pthread_cond_t wake;                     /* Signals queue or stop to the single owner. */
    HandlerFile *first;                      /* Oldest pending FID. */
    HandlerFile *last;                       /* Newest pending FID. */
    HandlerFile *active;                     /* FID executing on the sole owner, if any. */
    size_t queued;                           /* Jobs not yet started. */
    int stopped;                             /* Admission closed. */
    const StyxRpcChannelFunction *functions; /* Immutable borrowed copied-result descriptors. */
    const StyxRpcChannelExport *results;     /* Immutable borrowed leased-result descriptors. */
    size_t copied_count;                     /* Number of copied-result function nodes. */
    size_t count;                            /* Total exported function nodes. */
    StyxRpcModuleChannels *registry;        /* Optional registry owned by the dispatcher. */
    StyxRpcEntry entries[FIRST_EXPORT + STYXRPC_MAX_EXPORTS + 3]; /* Nodes, markers and optional bus. */
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES]; /* Serialized compiled offers. */
    size_t profile_size;                     /* Bytes in the immutable interface. */
};


/* EN: Queue the checked file once on its owner; return 0 if full or stopped.
 * RU: Однократно поставить проверенный file в очередь владельца. 0 при отказе. */
int styxrpc_handlers_submit(HandlerFile *file);


/* EN: Detach an executing file or remove it from the queue. Return 1 if caller owns it.
 * RU: Отсоединить активный файл либо снять его с очереди. 1 при передаче вызывающему. */
int styxrpc_handlers_detach(HandlerFile *file);


/* EN: Retire output ownership once without freeing its file.
 * RU: Единожды освободить результат, сохранив файл. */
void styxrpc_handlers_release_output(HandlerFile *file);


/* EN: Release an unpublished or already revoked output and the file that holds it.
 * RU: Освободить неопубликованный либо отозванный результат и его файл. */
void styxrpc_handlers_dispose(HandlerFile *file);

#endif
