/* FlashVM: named root slots for value-based method invocation and construction. */
#ifndef FVM_CALL_METHOD_CONSTANTS_H
#define FVM_CALL_METHOD_CONSTANTS_H /* Root-layout guard. */
/* Incoming owner survives boxing and lookup callbacks. */
#define FVM_METHOD_OWNER_ROOT 0U
/* Incoming counted method name survives normalization. */
#define FVM_METHOD_NAME_ROOT 1U
/* Anonymous-call receiver survives nested script collection. */
#define FVM_METHOD_THIS_ROOT 2U
/* Boxed primitive receiver is retained independently of its source value. */
#define FVM_METHOD_BOX_ROOT 3U
/* Constructor function returned by a getter is retained until invocation. */
#define FVM_METHOD_FUNCTION_ROOT 4U
/* Converted method text survives callbacks and counted-key normalization. */
#define FVM_METHOD_TEXT_ROOT 5U
/* Arguments occupy the remaining contiguous root slots. */
#define FVM_METHOD_ARGUMENT_ROOT_BASE 6U
#endif
