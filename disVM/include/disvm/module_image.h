/* Transport-independent, owned Dis images for the shared FlashVM host. */
#ifndef DISVM_MODULE_IMAGE_H
#define DISVM_MODULE_IMAGE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Match readmod's strict upper bound; quota is enforced before allocation/I/O. */
#define DVM_MODULE_IMAGE_MAX (8UL * 1024UL * 1024UL - 1UL)
/* Never ask a transport to deliver an unbounded native read. */
#define DVM_MODULE_READ_CHUNK 65536UL
/* Transport errors remain distinct from parser rejection and pending messages. */
typedef enum DVM_ImageResult {
    DVM_IMAGE_OK = 0,
    DVM_IMAGE_INVALID = -1,
    DVM_IMAGE_NOMEM = -2,
    DVM_IMAGE_IO = -3,
    DVM_IMAGE_TRUNCATED = -4,
    DVM_IMAGE_LIMIT = -5,
    DVM_IMAGE_PENDING = -6,
    DVM_IMAGE_BAD_MODULE = -7
} DVM_ImageResult;
/* Native byte storage, independent of either VM heap. Initialize to {0,0,0}.
 * [GC:] No VM pointer/root is stored here; the parser creates its own module. */
typedef struct DVM_ModuleImage {
    unsigned char *bytes; /* Owned mutable copy, including embedded zero bytes. */
    size_t size;         /* Exact accessible module length in bytes. */
    char *name;          /* Owned logical source identity; not an OS path policy. */
} DVM_ModuleImage;
/* Synchronous transport: 1 with 0..capacity bytes, 0 on error. Zero means EOF.
 * Short reads are legal. Callback must obey capacity and must not throw or enter
 * either VM. It owns cancellation/deadline policy and must eventually return. */
typedef int (*DVM_ModuleRead)(void *user, void *bytes, size_t capacity, size_t *received);
/* Deep-copy IDE memory or an embedded executable resource. Output must be empty.
 * Failure preserves it; no bytecode parsing or VM execution occurs here. */
DVM_ImageResult dvm_image_copy(const char *name, const void *bytes, size_t size,
                             size_t quota, DVM_ModuleImage *out);
/* Collect exactly size bytes, handling short reads. Caller frames the module;
 * trailing transport bytes are left unread for the next message. Never probe
 * EOF past size. Failure frees partial storage and preserves empty out. */
DVM_ImageResult dvm_image_read(const char *name, DVM_ModuleRead read, void *user,
                             size_t size, size_t quota, DVM_ModuleImage *out);
/* Release native storage; null and already-empty objects are safe. */
void dvm_image_dispose(DVM_ModuleImage *image);
/* The opaque result is an upstream Module, not an inter-VM message payload. */
typedef struct Module DVM_Module;
/* Invoke the retained production parsemod, not a second Dis parser. Requires
 * an initialized Dis host, owner VM lock, process error frames and builtin types.
 * [GC:] Returned module belongs to Dis; release with its unload() on that owner.
 * Image remains caller-owned and may be disposed after this synchronous call.
 * Name identifies a fresh load: no path cache/reload policy is implied. */
DVM_ImageResult dvm_image_parse(const DVM_ModuleImage *image, DVM_Module **out);
#ifdef __cplusplus
}
#endif
#endif
