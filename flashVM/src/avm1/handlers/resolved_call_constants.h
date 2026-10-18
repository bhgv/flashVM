/* FlashVM: root layout for already-resolved named calls and constructors. */
#ifndef FVM_RESOLVED_CALL_CONSTANTS_H
#define FVM_RESOLVED_CALL_CONSTANTS_H /* Private root-layout guard. */
#define FVM_RESOLVED_NAME_ROOT 0U     /* Counted fallback name retained through callbacks. */
#define FVM_RESOLVED_FUNCTION_ROOT                                                                 \
    1U /* Resolved callable/constructor retained through invocation. */
#define FVM_RESOLVED_RECEIVER_ROOT 2U /* Explicit call receiver retained through callbacks. */
#define FVM_RESOLVED_ARGUMENT_BASE 3U /* Ordered arguments occupy the remaining root slots. */
#endif
