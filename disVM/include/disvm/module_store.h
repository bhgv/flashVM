/* Versioned module bytes shared by compiler and loader owners; no VM heap pointers. */
#ifndef DISVM_MODULE_STORE_H
#define DISVM_MODULE_STORE_H

#include <limits.h>
#include "module_image.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DVM_STORE_NAME_MAX 255UL       /* Maximum logical namespace name in bytes, excluding NUL. */
#define DVM_STORE_ENTRIES_MAX 128UL    /* Hard bound on simultaneously published names. */
#define DVM_STORE_VERSIONS_MAX 512UL   /* Hard bound including retired versions pinned by readers. */
#define DVM_STORE_LEASES_MAX 4096UL    /* Hard bound on independent live reader handles. */
#define DVM_STORE_BYTES_MAX 67108864UL /* Hard bound on managed image bytes, excluding fixed metadata. */
#define DVM_STORE_ANY ULONG_MAX        /* Unconditional publication/removal; zero means create only. */


typedef struct DVM_ModuleStore DVM_ModuleStore; /* Opaque synchronized native namespace owner. */
typedef struct DVM_ModuleLease DVM_ModuleLease; /* Exclusive handle pinning immutable bytes. */


/* Native store status; transport/parser statuses remain separate. */
typedef enum DVM_StoreResult
{
    DVM_STORE_OK = 0,         /* Operation committed or a lease was returned. */
    DVM_STORE_INVALID = -1,   /* Invalid argument or noncanonical logical name. */
    DVM_STORE_NOMEM = -2,     /* Native allocation or synchronization allocation failed. */
    DVM_STORE_LIMIT = -3,     /* Image, byte, name, version or reader quota exceeded. */
    DVM_STORE_NOT_FOUND = -4, /* No current image is published under this name. */
    DVM_STORE_CONFLICT = -5,  /* Expected generation does not match the current publication. */
    DVM_STORE_CLOSED = -6,    /* Close has prohibited further acquisitions and writes. */
    DVM_STORE_BUSY = -7,      /* Destruction refused because leases still exist. */
    DVM_STORE_EXHAUSTED = -8, /* A generation counter cannot advance without wrapping. */
    DVM_STORE_SOURCE = -9     /* Explicit provider failed; its detailed status is returned separately. */
} DVM_StoreResult;


/* Immutable borrowed view; all pointers expire when its lease is released. */
typedef struct DVM_ModuleView
{
    const unsigned char *bytes; /* Exact immutable module image; never a VM heap address. */
    size_t size;                /* Accessible byte extent. */
    const char *name;           /* Canonical logical name; not an OS path or sandbox permission. */
    unsigned long generation;   /* Unique store-lifetime publication ID, never zero or DVM_STORE_ANY. */
} DVM_ModuleView;


/* Copied listing entry, independent of subsequent publication or lease release. */
typedef struct DVM_ModuleEntry
{
    char name[DVM_STORE_NAME_MAX + 1]; /* NUL-terminated logical namespace name. */
    size_t size;                       /* Current image size in bytes. */
    unsigned long generation;          /* Current immutable version ID. */
} DVM_ModuleEntry;


/* Snapshot taken under the store mutex; byte accounting includes pinned retired versions. */
typedef struct DVM_ModuleStoreStats
{
    size_t names;         /* Published names currently visible to acquire/list. */
    size_t versions;      /* Current and retired resident versions. */
    size_t leases;        /* Reader handles preventing store destruction. */
    size_t bytes;         /* Managed image bytes, not temporary copies or allocator overhead. */
    unsigned long latest; /* Most recently published generation; initially zero. */
    int closed;           /* Whether new acquisitions and publications are prohibited. */
} DVM_ModuleStoreStats;


/* Provider runs without the store lock; it may block on a file/socket/channel.
 * user and name are borrowed for the call; out starts empty and must use the
 * module_image ownership contract. Return a transport result; no VM exceptions.
 * The store disposes partial/complete out on every path. The provider may reenter
 * this store, but may not destroy it; its caller retains the store lifetime. */
typedef DVM_ImageResult (*DVM_ModuleProvider)(void *user, const char *name, DVM_ModuleImage *out);


/* Create a native namespace, with 1..ENTRIES_MAX names and 1..BYTES_MAX bytes.
 * Returns an owner or NULL. All users must quiesce before successful destroy.
 * Names are opaque byte identifiers with POSIX separators: absolute, no empty,
 * dot/dot-dot, backslash, colon or ASCII-control segments; max NAME_MAX bytes.
 * The store performs no filesystem traversal and grants no path permissions. */
DVM_ModuleStore *dvm_module_store_create(size_t max_names, size_t max_bytes);


/* Copy image into a new version. expected is zero for create-only, a generation
 * for compare-and-swap, or ANY. out_generation changes only on success. The image
 * remains caller-owned and stable for this call. Failures preserve current data.
 * Replacement retains old reader bytes and charges them against the byte quota. */
DVM_StoreResult dvm_module_store_publish(DVM_ModuleStore *store,
                                         const DVM_ModuleImage *image,
                                         unsigned long expected,
                                         unsigned long *out_generation);


/* Pin the current image. out must initially be NULL; failure preserves it.
 * The caller owns one exclusive lease and must release it exactly once. */
DVM_StoreResult dvm_module_store_acquire(DVM_ModuleStore *store, const char *name, DVM_ModuleLease **out);


/* Copy a borrowed view from a live lease. Returns INVALID for NULL inputs.
 * Neither lease release nor destruction may race with use of its borrowed view. */
DVM_StoreResult dvm_module_lease_view(const DVM_ModuleLease *lease, DVM_ModuleView *out);


/* Release one exclusive lease and clear *lease. NULL/already-cleared is safe.
 * Old versions are freed after their final lease; store destruction is then retryable. */
void dvm_module_lease_release(DVM_ModuleLease **lease);


/* Unpublish a name after checking expected (ANY or its current generation).
 * Existing readers remain valid; missing names return NOT_FOUND. No VM unload occurs. */
DVM_StoreResult dvm_module_store_remove(DVM_ModuleStore *store, const char *name, unsigned long expected);


/* Copy every current entry atomically; order is unspecified. needed is set to the
 * required count on OK/LIMIT. Insufficient capacity leaves entries unchanged.
 * entries may be NULL only with zero capacity. Failure otherwise preserves outputs. */
DVM_StoreResult
dvm_module_store_list(DVM_ModuleStore *store, DVM_ModuleEntry *entries, size_t capacity, size_t *needed);


/* Copy synchronized counters. Returns INVALID for NULL arguments, otherwise OK. */
DVM_StoreResult dvm_module_store_stats(DVM_ModuleStore *store, DVM_ModuleStoreStats *out);


/* Load through an explicit provider, then publish with the same CAS contract.
 * source_status receives the provider result; generation changes only on success.
 * Source work happens outside locks and may lose a race: CONFLICT prevents stale
 * IDE builds from overwriting a newer publication. Provider names must match name. */
DVM_StoreResult dvm_module_store_load(DVM_ModuleStore *store,
                                      const char *name,
                                      unsigned long expected,
                                      DVM_ModuleProvider provider,
                                      void *user,
                                      DVM_ImageResult *source_status,
                                      unsigned long *out_generation);


/* Prohibit further writes/acquisitions; existing leases and stats remain usable.
 * Close is idempotent. Caller must separately stop/join all store users. */
void dvm_module_store_close(DVM_ModuleStore *store);


/* Destroy an externally quiesced store. BUSY preserves it while leases exist.
 * A successful call clears *store. NULL/already-cleared returns OK. This is not
 * concurrent lifetime management: destroy must not race with any store operation. */
DVM_StoreResult dvm_module_store_destroy(DVM_ModuleStore **store);

#ifdef __cplusplus
}
#endif
#endif
