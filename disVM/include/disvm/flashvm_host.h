/* Optional Dis adapters over the actual FlashVM I/O and OS-thread interfaces. */
#ifndef DISVM_FLASHVM_HOST_H
#define DISVM_FLASHVM_HOST_H
#include "module_image.h"
#include "flashvm/channel.h"
#include "thread_port.h"
#include "channel_sync.h"
#include "network/socket.h"

/* Versioned binary-string envelope; ASCII avoids WTF-8 coercion/data loss. */
#define DVM_MODULE_CHANNEL_PREFIX "dis-hex:"
#define DVM_MODULE_CHANNEL_PREFIX_SIZE 8U
#ifdef __cplusplus
extern "C"
{
#endif


/* Read a framed module through FlashVM's portable file layer, then close only
 * the handle opened here. Exact size comes from the caller's resource metadata. */
DVM_ImageResult dvm_image_file(const char *name, const char *path, size_t size, size_t quota, DVM_ModuleImage *out);
/* Borrow an already connected FlashVM socket; retain its total timeout and
 * cancellation policy. Never open a second networking stack or close the socket.
 * Call on an I/O worker, not the VM/render owner thread. Framing size is required. */
DVM_ImageResult
dvm_image_socket(const char *name, FVM_NetSocket *socket, size_t size, size_t quota, DVM_ModuleImage *out);
/* Receive one dis-hex envelope, preserving arbitrary Dis bytes. blocking=0 returns
 * PENDING for an empty/open channel. Invalid/oversized messages are consumed and
 * released; caller must bound the producer/channel's independent memory quota.
 * This is the native image transport, not the typed Dis rendezvous ABI or Styx. */
DVM_ImageResult
dvm_image_channel(const char *name, FVM_Channel *channel, int blocking, size_t quota, DVM_ModuleImage *out);


/* Send one bounded binary module using the envelope above; size doubles on wire.
 * blocking=0 returns PENDING when full, leaving the image unchanged for retry.
 * Caller owns image and channel throughout; no VM execution occurs here. */
DVM_ImageResult dvm_image_send_channel(FVM_Channel *channel, const DVM_ModuleImage *image, int blocking);


/* Shared native execution primitives, with exactly FlashVM's platform handles.
 * No per-Limbo-process OS thread is implied: the Dis scheduler runs above this.
 * join consumes the thread handle; sync wait releases/reacquires its held lock.
 * Callback state must outlive join. Do not self-join or destroy a live sync. */
FVM_PortThread *dvm_host_thread_start(FVM_PortThreadFn callback, void *user);


void dvm_host_thread_join(FVM_PortThread *thread);


FVM_PortSync *dvm_host_sync_create(void);


void dvm_host_sync_destroy(FVM_PortSync *sync);


void dvm_host_sync_lock(FVM_PortSync *sync);


void dvm_host_sync_unlock(FVM_PortSync *sync);


void dvm_host_sync_wait(FVM_PortSync *sync);


void dvm_host_sync_wake_all(FVM_PortSync *sync);
#ifdef __cplusplus
}
#endif
#endif
