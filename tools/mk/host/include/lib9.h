/* POSIX declarations from the selected upstream SDK. The ARM-only inline
 * return-address helper is not used on this host; host/support.c supplies
 * the native compiler intrinsic instead. No upstream blob is edited. */
#ifndef FLASHVM_MK_HOST_LIB9_H
#define FLASHVM_MK_HOST_LIB9_H
#define getcallerpc disvm_unused_arm_callerpc
#include "../../upstream/Linux/arm/include/lib9.h"

#undef getcallerpc
/* mk uses native time_t declarations on POSIX hosts. */
#include <time.h>


extern uintptr getcallerpc(void *);
#endif
