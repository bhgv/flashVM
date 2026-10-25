/* OS: all host outputs. Translate one callback failure without skipping independent stops. */
#include "channel.hpp" /* Reuse the native callback ABI. */
#include <new>         /* Distinguish allocation failure from other native exceptions. */


/* Invoke only under a caller-owned FVM_AudioCallbackGuard; allocate no VM state. */
FVM_Result fvm_audio_invoke_stop(FVM_AudioStopCallback callback, void *userdata) /* Borrow native host state. */
{                                                                                /* Begin the per-callback exception boundary. */
    try                                                                          /* Preserve every explicit host result unchanged. */
    {                                                                            /* Begin exception boundary: Preserve every explicit host result unchanged. */
        return callback(userdata);                                               /* Preserve every explicit host result unchanged. */
    } /* Preserve every explicit host result unchanged. */
    catch (const std::bad_alloc &) /* Keep native OOM retryable. */
    {                              /* Begin exception boundary: Keep native OOM retryable. */
        return FVM_OUT_OF_MEMORY;  /* Keep native OOM retryable. */
    } /* Keep native OOM retryable. */
    catch (...)           /* Never unwind across the public C ABI or context teardown. */
    {                     /* Begin exception boundary: Never unwind across the public C ABI or context teardown. */
        return FVM_ERROR; /* Never unwind across the public C ABI or context teardown. */
    } /* Never unwind across the public C ABI or context teardown. */
} /* End one independent stop attempt. */
