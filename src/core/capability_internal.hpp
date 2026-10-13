/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: capability internal.
 * File: src/core/capability_internal.hpp.
 */

#ifndef FVM_CAPABILITY_INTERNAL_HPP
/* Define FVM CAPABILITY INTERNAL HPP constant or compilation switch. */
#define FVM_CAPABILITY_INTERNAL_HPP
#include "runtime.hpp"
#include "flashvm/mutex.h"
#include "flashvm/channel.h"
/* Implement capability export for this subsystem. */
FVM_Result fvm_capability_export(FVM_Context *, FVM_Object *, fvm_detail::string &);

/* Implement capability import for this subsystem. */
FVM_Result fvm_capability_import(FVM_Context *, const char *, unsigned long, FVM_Object **);

/* Implement capability context cleanup for this subsystem. */
void fvm_capability_context_cleanup(FVM_Context *);

/* Implement debug resource error for this subsystem. */
void fvm_debug_resource_error(FVM_Context *, unsigned long, const char *, const char *);
/* Remote operation codes: 0 status, 1 set, 2 reset, 3 touch, 4 close.
   The callback performs a single nonblocking acquisition attempt. */
typedef FVM_Result (*FVM_RemoteMutexFn)(void *, FVM_Context *, int, int *);

/* Create mutex remote. */
FVM_Result fvm_mutex_create_remote(FVM_Context *, void *, FVM_RemoteMutexFn, void (*)(void *),
                                   const char *, FVM_Mutex **);

/* Implement mutex touch context for this subsystem. */
FVM_Result fvm_mutex_touch_context(FVM_Context *, FVM_Mutex *, int *);

/* Return the remote endpoint address associated with this mutex. */
const char *fvm_mutex_remote_address(FVM_Mutex *);
#include "channel_packet.h"
/* Send channel wire. */
FVM_Result fvm_channel_send_wire(FVM_Channel *, const unsigned char *, size_t);

/* Receive channel wire. */
FVM_Result fvm_channel_receive_wire(FVM_Context *, FVM_Channel *, unsigned char **, size_t *);
/* Encode an indexed queue snapshot without removing its packet or shared resources. */
FVM_Result fvm_channel_peek_wire(FVM_Context *, FVM_Channel *, size_t, unsigned char **, size_t *);
#endif
