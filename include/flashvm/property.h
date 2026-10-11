#ifndef FLASHVM_PROPERTY_H
#define FLASHVM_PROPERTY_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_PROPERTY_DONT_ENUM 1
#define FVM_PROPERTY_DONT_DELETE 2
#define FVM_PROPERTY_READ_ONLY 4
    FVM_API FVM_Result fvm_object_property_flags(FVM_Context *, FVM_Object *, const char *,
                                                 unsigned int *);
    FVM_API FVM_Result fvm_object_set_property_flags(FVM_Context *, FVM_Object *, const char *,
                                                     unsigned int set, unsigned int clear);
    FVM_API FVM_Result fvm_object_delete(FVM_Context *, FVM_Object *, const char *, int *deleted);
#ifdef __cplusplus
}
#endif
#endif
