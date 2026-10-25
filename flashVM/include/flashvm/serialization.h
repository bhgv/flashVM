/* FlashVM: Public embedding API declarations: serialization.
 * File: include/flashvm/serialization.h.
 */

#ifndef FLASHVM_SERIALIZATION_H
/* Define FLASHVM SERIALIZATION H constant or compilation switch. */
#define FLASHVM_SERIALIZATION_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Owned, versioned graph snapshots. Data contains no process/VM pointers.
     * Preserve cycles, shared edges, prototypes, metatables and registered callbacks.
     * Limits: 4096 objects, 65536 entries, 64 MiB including the envelope.
     * Free successful output exactly once using fvm_serialized_free. */


/* Serialize value. */
FVM_API FVM_Result fvm_value_serialize(FVM_Context *, const FVM_Value *, unsigned char **data,
                                       size_t *size);


/* Deserialize value. */
FVM_API FVM_Result fvm_value_deserialize(FVM_Context *, const void *data, size_t size,
                                         FVM_Value *out);


/* Implement serialized free for this subsystem. */
FVM_API void fvm_serialized_free(void *data);
/* Register the same stable code identity in sender and receiver contexts.
     * Callback userdata must be null or exactly its traced root; roots are cloned.
     * Registration binds code only, never serializes arbitrary native memory.
     * Names beginning with "flashvm." are reserved for built-in codecs. */


/* Register serialization callback. */
FVM_API FVM_Result fvm_serialization_register_callback(FVM_Context *, const char *name,
                                                       FVM_NativeFunction);
#ifdef __cplusplus
}
#endif
#endif
