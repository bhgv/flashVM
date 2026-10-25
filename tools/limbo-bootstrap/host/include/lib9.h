/* POSIX declarations from the selected upstream SDK. The ARM-only inline
 * return-address helper is not used on this host; bootstrap_support supplies
 * the native compiler intrinsic instead. No upstream blob is edited. */
#ifndef FLASHVM_HOST_LIB9_H
#define FLASHVM_HOST_LIB9_H
#define getcallerpc disvm_unused_arm_callerpc
#include "../../upstream/Linux/arm/include/lib9.h"

#undef getcallerpc


extern uintptr getcallerpc(void *);
#endif
