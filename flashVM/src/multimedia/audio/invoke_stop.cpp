/* OS: all host outputs. Translate one callback failure without skipping independent stops. */
#include "channel.hpp" /* Reuse the native callback ABI. */
#include <new> /* Distinguish allocation failure from other native exceptions. */
/* Invoke only under a caller-owned FVM_AudioCallbackGuard; allocate no VM state. */
FVM_Result fvm_audio_invoke_stop(FVM_AudioStopCallback callback, void *userdata) /* Borrow native host state. */
{ /* Begin the per-callback exception boundary. */
    try { return callback(userdata); } /* Preserve every explicit host result unchanged. */
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; } /* Keep native OOM retryable. */
    catch (...) { return FVM_ERROR; } /* Never unwind across the public C ABI or context teardown. */
} /* End one independent stop attempt. */
