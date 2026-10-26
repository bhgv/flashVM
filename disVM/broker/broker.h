/* Bounded, value-only RPC diagnostics. All VM objects remain with their owner. */
#ifndef DVM_BROKER_H
#define DVM_BROKER_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define DVM_BROKER_CHANNELS 16  /* Maximum registered external channels per owner broker. */
#define DVM_BROKER_SITES 128    /* Immutable source-site records, shared by requests. */
#define DVM_BROKER_REQUESTS 128 /* Requests retained until explicitly forgotten after completion. */
#define DVM_BROKER_LISTENERS 8  /* Independent diagnostic subscriptions, including global listeners. */
#define DVM_BROKER_EVENTS 32    /* Bounded queued deviations per listener; success is never queued. */


/* Native API status, separate from remote completion and diagnostic reasons. */
typedef enum
{
    DVM_BROKER_OK,      /* Successful local operation. */
    DVM_BROKER_EMPTY,   /* No diagnostic is waiting. */
    DVM_BROKER_INVALID, /* Invalid arguments or backward monotonic time. */
    DVM_BROKER_STALE,   /* Unknown/revoked handle or obsolete session generation. */
    DVM_BROKER_LIMIT,   /* Bounded storage or ID space exhausted. */
    DVM_BROKER_BUSY     /* An executing request cannot be forgotten. */
} DVM_BrokerResult;


/* Origin coordinates are UTF-8 byte coordinates, not screen/UTF-16 columns. */
typedef struct
{
    char build[65];   /* Caller-supplied immutable build identity, normally a hexadecimal digest. */
    char path[256];   /* Copied source path, never a borrowed pointer. */
    char module[128]; /* Copied module/function description. */
    uint32_t line;    /* One-based source line. */
    uint32_t column;  /* One-based UTF-8 byte column. */
    uint64_t offset;  /* Zero-based UTF-8 byte offset in this exact build's source. */
    uint32_t length;  /* Exact operation/token byte length; repeated names have separate sites. */
    uint32_t binding; /* Compiler binding identity, zero when unavailable. */
} DVM_BrokerSite;


/* A generation distinguishes reconnects even when a channel identity is retained. */
typedef struct
{
    uint32_t channel;    /* Opaque broker-local external channel ID. */
    uint32_t generation; /* Session generation; zero is invalid. */
    uint64_t request;    /* Monotonic request ID, never reused during this broker lifetime. */
} DVM_BrokerKey;


/* Transport class is metadata; no network operation runs while holding the broker lock. */
typedef enum
{
    DVM_BROKER_IN_PROCESS = 1, /* Another interpreter/owner in this process. */
    DVM_BROKER_IPC,            /* Another process through an explicit IPC adapter. */
    DVM_BROKER_NETWORK         /* Network endpoint supplied by a transport adapter. */
} DVM_BrokerTransport;


/* Copied channel card; requests preserve this card across later reconnects. */
typedef struct
{
    uint32_t id;                   /* Opaque channel identity. */
    uint32_t generation;           /* Session captured for this card. */
    DVM_BrokerTransport transport; /* Actual adapter transport class. */
    char logical[64];              /* Stable logical service name, independent of the endpoint. */
    char local[128];               /* Copied local address/IPC endpoint description. */
    char peer[128];                /* Copied protocol/address:port or IPC endpoint, excluding credentials. */
    uint32_t created_at;           /* Immutable site registry ID where the channel was created. */
    uint32_t connected_at;         /* Site registry ID for this particular connection. */
    uint64_t heartbeat_ms;         /* Maximum peer silence; zero disables automatic liveness expiry. */
    uint64_t last_peer_ms;         /* Last observation reported by the transport adapter. */
    int open;                      /* Nonzero while this session admits requests. */
} DVM_BrokerChannel;


/* Owner-published state; a broker query does not enter the paused application's queue. */
typedef enum
{
    DVM_BROKER_QUEUED,     /* Waiting for admission/execution; does not prove nonexecution remotely. */
    DVM_BROKER_RUNNING,    /* Executing with a last reported progress point. */
    DVM_BROKER_DISK,       /* Waiting on disk/file I/O. */
    DVM_BROKER_NETWORK_IO, /* Waiting on network I/O. */
    DVM_BROKER_RPC,        /* Waiting for another RPC, named by parent/dependency metadata. */
    DVM_BROKER_LOCK,       /* Waiting on logical owner exclusivity. */
    DVM_BROKER_PAUSED,     /* Application task paused by debugger; broker remains runnable. */
    DVM_BROKER_COMPLETED,  /* Normal result is available through the ordinary response path. */
    DVM_BROKER_FAILED      /* Owner reported failure through the ordinary response path. */
} DVM_BrokerPhase;


/* Deviations only: normal acceptance/completion/heartbeat never becomes a diagnostic. */
typedef enum
{
    DVM_BROKER_SLOW = 1,     /* First warning threshold, or meaningful reason change after it. */
    DVM_BROKER_TIMED_OUT,    /* Caller deadline expired; execution may still complete. */
    DVM_BROKER_DISCONNECTED, /* Session revoked; remote execution outcome may be unknown. */
    DVM_BROKER_DEBUG_PAUSED, /* Debugger pause reported by the independent broker. */
    DVM_BROKER_OWNER_FAILED  /* Failed operation, with copied owner-provided detail. */
} DVM_BrokerEventKind;


/* Request record contains values only; snapshot queries never call getters or native objects. */
typedef struct
{
    DVM_BrokerKey key;         /* Correlation and immutable session identity. */
    DVM_BrokerKey parent;      /* Calling/dependent request, or all zero when unavailable. */
    DVM_BrokerChannel channel; /* Endpoint/session at dispatch, unaffected by reconnect. */
    DVM_BrokerPhase phase;     /* Last state explicitly published by the owner adapter. */
    uint32_t site;             /* Exact initiating source operation, not merely the variable name. */
    uint64_t object;           /* Opaque proxy/object identity, not a VM pointer. */
    uint64_t started_ms;       /* Monotonic dispatch time. */
    uint64_t updated_ms;       /* Monotonic last owner state/progress update. */
    uint64_t warn_ms;          /* Warning delay from start; zero disables slow warnings. */
    uint64_t deadline_ms;      /* Absolute caller deadline; zero means no deadline. */
    uint64_t progress;         /* Owner-defined monotonic progress counter, not fabricated activity. */
    int timed_out;             /* Caller deadline expired; owner completion remains independently tracked. */
    int disconnected;          /* Original session was revoked; late replies cannot enter a new session. */
    int finished;              /* Owner acknowledged completion; only then may the record be forgotten. */
    char operation[64];        /* ReadAs/Get/Set/Invoke/etc., supplied by an adapter. */
    char detail[128];          /* Last owner reason; freshness is given by updated_ms. */
} DVM_BrokerSnapshot;


/* One copied deviation with immutable origin and channel creation/connection sites. */
typedef struct
{
    DVM_BrokerEventKind kind;   /* Deviation class. */
    uint64_t at_ms;             /* Broker observation time, distinct from last owner progress. */
    DVM_BrokerSnapshot request; /* Exact request snapshot, zero request ID for channel-only loss. */
    DVM_BrokerSite origin;      /* Initiating expression/token; zero when channel-only. */
    DVM_BrokerSite creation;    /* Channel creation source. */
    DVM_BrokerSite connection;  /* Connection source for the captured generation. */
} DVM_BrokerEvent;


/* Borrowed native synchronization. These callbacks protect short metadata copies only. */
typedef struct
{
    void (*enter)(void *user); /* Lock a live native mutex; never enter a VM or call the broker recursively. */
    void (*leave)(void *user); /* Unlock the matching native mutex. */
    void *user;                /* Borrowed lock owner, valid until broker destruction after quiescence. */
} DVM_BrokerSync;

typedef struct DVM_Broker DVM_Broker; /* Owns bounded registries and copied event queues. */


/* Allocate one broker using borrowed sync; out is unchanged on error. No callbacks are invoked here. */
DVM_BrokerResult dvm_broker_create(const DVM_BrokerSync *sync, DVM_Broker **out);


/* Free native metadata after all API users/diagnostic workers have quiesced; null is harmless. */
void dvm_broker_destroy(DVM_Broker *broker);


/* Copy one immutable origin. source/id are borrowed/returned; no native or VM pointer is retained. */
DVM_BrokerResult dvm_broker_site(DVM_Broker *broker, const DVM_BrokerSite *source, uint32_t *id);


/* Register a channel card, assigning id/generation=1; validated sites must already exist.
 * now_ms is monotonic. card/out are borrowed/returned; outputs are unchanged on rejection. */
DVM_BrokerResult dvm_broker_open(DVM_Broker *broker, const DVM_BrokerChannel *card, uint64_t now_ms, uint32_t *out);


/* Reconnect a closed channel with a new endpoint, connection origin and generation.
 * Other card fields stay fixed. Pending old records retain their previous endpoint metadata. */
DVM_BrokerResult dvm_broker_reconnect(
    DVM_Broker *broker, uint32_t channel, const char *local, const char *peer, uint32_t site, uint64_t now_ms);


/* Publish actual peer liveness without a success event; stale generations are rejected. */
DVM_BrokerResult dvm_broker_heartbeat(DVM_Broker *broker, uint32_t channel, uint32_t generation, uint64_t now_ms);


/* Revoke one session and diagnose pending requests; does not pretend to stop remote execution. */
DVM_BrokerResult dvm_broker_close(DVM_Broker *broker, uint32_t channel, uint32_t generation, uint64_t now_ms);


/* Admit a request from template into the current channel session; assign key and timing.
 * template key.channel/generation/site/operation/object/parent/warn/deadline are inputs; other fields ignored.
 * No success event or object evaluation occurs. deadline must be zero or greater than now. */
DVM_BrokerResult
dvm_broker_begin(DVM_Broker *broker, const DVM_BrokerSnapshot *request, uint64_t now_ms, DVM_BrokerKey *key);


/* Owner publishes live phase/reason/progress. No callback/getter is invoked and no success is emitted.
 * Progress cannot decrease. Updated state remains queryable while the application is paused. */
DVM_BrokerResult dvm_broker_progress(DVM_Broker *broker,
                                     DVM_BrokerKey key,
                                     DVM_BrokerPhase phase,
                                     uint64_t progress,
                                     const char *detail,
                                     uint64_t now_ms);


/* Acknowledge owner completion, including after timeout/revocation, so roots can later be retired.
 * failed is 0/1. A revoked session or expired caller returns STALE after recording completion; suppress its late reply.
 * The transport adapter owns actual result delivery and all VM roots. Success emits no diagnostic. */
DVM_BrokerResult
dvm_broker_finish(DVM_Broker *broker, DVM_BrokerKey key, int failed, const char *detail, uint64_t now_ms);


/* Copy diagnostic state under only the broker mutex; no application lock or owner RPC is taken. */
DVM_BrokerResult dvm_broker_query(DVM_Broker *broker, DVM_BrokerKey key, DVM_BrokerSnapshot *out);


/* Retire a finished record. Timeout/disconnect alone is not owner completion and returns BUSY. */
DVM_BrokerResult dvm_broker_forget(DVM_Broker *broker, DVM_BrokerKey key);


/* Scan bounded metadata for warning/deadline/heartbeat deviations. now must not move backward.
 * The host runs this on its independent broker pump, even while application tasks are paused. */
DVM_BrokerResult dvm_broker_tick(DVM_Broker *broker, uint64_t now_ms);


/* Subscribe to deviations: channel=0/request=0 observes every external channel in this broker.
 * Nonzero fields filter exact channel/request IDs. Each listener has its own bounded queue. */
DVM_BrokerResult dvm_broker_watch(DVM_Broker *broker, uint32_t channel, uint64_t request, uint32_t *listener);


/* Remove a subscription and its native queued copies; no VM/channel operation is performed. */
DVM_BrokerResult dvm_broker_unwatch(DVM_Broker *broker, uint32_t listener);


/* Poll a copied event without waiting. dropped returns/reset queue-overflow count, including on EMPTY.
 * Event out remains unchanged on EMPTY/errors. Listener overflow never blocks RPC or broker work. */
DVM_BrokerResult dvm_broker_poll(DVM_Broker *broker, uint32_t listener, DVM_BrokerEvent *out, uint64_t *dropped);


/* Copy the current transport/session card; out is unchanged for an unknown identity. */
DVM_BrokerResult dvm_broker_channel(DVM_Broker *broker, uint32_t channel, DVM_BrokerChannel *out);
#ifdef __cplusplus
}
#endif
#endif
