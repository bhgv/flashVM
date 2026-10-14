/* FlashVM: borrowed string representation markers. */
#ifndef FVM_CORE_STRING_CONSTANTS_H
#define FVM_CORE_STRING_CONSTANTS_H /* Internal string representation guard. */
#include <stddef.h>
/* A borrowed value with this size uses a terminating NUL instead of an explicit count. */
#define FVM_STRING_TERMINATED_SIZE ((size_t)-1)
#endif
