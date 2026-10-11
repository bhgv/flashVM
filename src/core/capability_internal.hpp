#ifndef FVM_CAPABILITY_INTERNAL_HPP
#define FVM_CAPABILITY_INTERNAL_HPP
#include "runtime.hpp"
#include "flashvm/mutex.h"
#include "flashvm/channel.h"
FVM_Result fvm_capability_export(FVM_Context *, FVM_Object *, fvm_detail::string &);
FVM_Result fvm_capability_import(FVM_Context *, const char *, unsigned long, FVM_Object **);
void fvm_capability_context_cleanup(FVM_Context *);
void fvm_debug_resource_error(FVM_Context *, unsigned long, const char *, const char *);
/* Remote operation codes: 0 status, 1 set, 2 reset, 3 touch, 4 close.
   The callback performs a single nonblocking acquisition attempt. */
typedef FVM_Result (*FVM_RemoteMutexFn)(void *, FVM_Context *, int, int *);
FVM_Result fvm_mutex_create_remote(FVM_Context *, void *, FVM_RemoteMutexFn, void (*)(void *),
                                   const char *, FVM_Mutex **);
FVM_Result fvm_mutex_touch_context(FVM_Context *, FVM_Mutex *, int *);
const char *fvm_mutex_remote_address(FVM_Mutex *);
#include "channel_packet.h"
FVM_Result fvm_channel_send_wire(FVM_Channel *, const unsigned char *, size_t);
FVM_Result fvm_channel_receive_wire(FVM_Context *, FVM_Channel *, unsigned char **, size_t *);
#endif
