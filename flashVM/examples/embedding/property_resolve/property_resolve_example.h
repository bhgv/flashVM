/* FlashVM: named counts for the C89 resolver tutorial. */
#ifndef FVM_PROPERTY_RESOLVE_EXAMPLE_H /* Prevent repeated tutorial definitions. */
#define FVM_PROPERTY_RESOLVE_EXAMPLE_H /* Mark the tutorial header as included. */
#define FVM_RESOLVE_EXAMPLE_METHOD "sum" /* Resolve this complete method name. */
#define FVM_RESOLVE_EXAMPLE_NAME_BYTES (sizeof(FVM_RESOLVE_EXAMPLE_METHOD) - 1U) /* Exclude the C terminator. */
#define FVM_RESOLVE_EXAMPLE_NAME_ARGUMENTS 1U /* __resolve receives one requested name. */
#define FVM_RESOLVE_EXAMPLE_SUM_ARGUMENTS 2U /* The original C sum takes two Numbers. */
#endif /* Finish the tutorial constants. */
