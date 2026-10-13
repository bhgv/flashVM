/* FlashVM: Public embedding API declarations: property.
 * File: include/flashvm/property.h.
 */

#ifndef FLASHVM_PROPERTY_H
/* Define FLASHVM PROPERTY H constant or compilation switch. */
#define FLASHVM_PROPERTY_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Define FVM PROPERTY DONT ENUM constant or compilation switch. */
#define FVM_PROPERTY_DONT_ENUM 1
/* Define FVM PROPERTY DONT DELETE constant or compilation switch. */
#define FVM_PROPERTY_DONT_DELETE 2
/* Define FVM PROPERTY READ ONLY constant or compilation switch. */
#define FVM_PROPERTY_READ_ONLY 4
    /* Implement object property flags for this subsystem. */
    FVM_API FVM_Result fvm_object_property_flags(FVM_Context *, FVM_Object *, const char *,
                                                 unsigned int *);
    /* Update object property flags. */
    FVM_API FVM_Result fvm_object_set_property_flags(FVM_Context *, FVM_Object *, const char *,
                                                     unsigned int set, unsigned int clear);
    /* Read/update attributes for a counted WTF-8 property name. */
    FVM_API FVM_Result fvm_object_property_flags_bytes(FVM_Context *, FVM_Object *, const char *,
                                                       size_t, unsigned int *);
    FVM_API FVM_Result fvm_object_set_property_flags_bytes(FVM_Context *, FVM_Object *, const char *,
                                                           size_t, unsigned int, unsigned int);
    /* Delete a counted property name, respecting DONT_DELETE. */
    FVM_API FVM_Result fvm_object_delete_bytes(FVM_Context *, FVM_Object *, const char *, size_t,
                                               int *);
    /* Snapshot enumerable keys without invoking getters. Own nonenumerable
     * names shadow prototypes. Hidden watch descriptors are never keys.
     * The returned Array is borrowed; pin it while retained by native code. */
    /* Implement object enumerate for this subsystem. */
    FVM_API FVM_Result fvm_object_enumerate(FVM_Context *, FVM_Object *, int include_prototypes,
                                            FVM_Object **);
    /* Delete object. */
    FVM_API FVM_Result fvm_object_delete(FVM_Context *, FVM_Object *, const char *, int *deleted);
#ifdef __cplusplus
}
#endif
#endif
