#ifndef FLASHVM_USERDATA_H
#define FLASHVM_USERDATA_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Release callback owns the payload after successful attachment. It runs once
   when the object is actually reclaimed or its context is destroyed, after
   __gc/resurrection decisions. It must not access reclaimed VM objects.
   Object allocation and reentrant destruction/collection are rejected while
   release runs. Captured VM objects must be stored in traced properties. */
typedef void (*FVM_UserdataRelease)(void *);
FVM_API FVM_Result fvm_userdata_attach(FVM_Context *,FVM_Object *,void *,FVM_UserdataRelease);
FVM_API FVM_Result fvm_userdata_get(FVM_Context *,FVM_Object *,void **);
#ifdef __cplusplus
}
#endif
#endif
