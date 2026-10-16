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
/* Hide the property from enumerable-key snapshots. */
#define FVM_PROPERTY_DONT_ENUM 1
/* Prevent deletion until this flag is cleared. */
#define FVM_PROPERTY_DONT_DELETE 2
/* Ignore writes through the ordinary property interface. */
#define FVM_PROPERTY_READ_ONLY 4
/* Embedding setters accept these three bits; loaded ASSetPropFlags may store UInt16 attributes. */
#define FVM_PROPERTY_VALID_FLAGS (FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE | FVM_PROPERTY_READ_ONLY)
/* Default attributes for built-in raw prototype slots. */
#define FVM_PROPERTY_BUILTIN_FLAGS (FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE)
    /* Query stored attributes, including UInt16 version bits set by loaded AVM1. */
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
