/* FlashVM: temporary native enumeration root slots. */
#ifndef FVM_JIT_ENUMERATE_CONSTANTS_H
#define FVM_JIT_ENUMERATE_CONSTANTS_H /* Native enumeration root layout guard. */
#define FVM_JIT_ENUMERATE_NAME_ROOT 0U /* Original input and then converted binding name. */
#define FVM_JIT_ENUMERATE_TARGET_ROOT 1U /* Resolved object or direct input value. */
#define FVM_JIT_ENUMERATE_SNAPSHOT_ROOT 2U /* Dense native Array of property names. */
#define FVM_JIT_ENUMERATE_ROOT_COUNT 3U /* [GC:] Name, target and snapshot remain live together. */
#endif
