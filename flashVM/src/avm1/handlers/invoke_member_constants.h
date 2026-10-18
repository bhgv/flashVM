/* FlashVM: named member invocation roots independent of operand-stack layout. */
#ifndef FVM_INVOKE_MEMBER_CONSTANTS_H
#define FVM_INVOKE_MEMBER_CONSTANTS_H      /* Member invocation constants guard. */
#define FVM_INVOKE_MEMBER_RECEIVER_ROOT 0U /* [GC:] Actual receiver through getter and body. */
#define FVM_INVOKE_MEMBER_NAME_ROOT 1U     /* [GC:] Already converted counted member name. */
#define FVM_INVOKE_MEMBER_FUNCTION_ROOT 2U /* [GC:] Single selected getter result. */
#define FVM_INVOKE_MEMBER_SOURCE_ROOT 3U   /* [GC:] Defining prototype captured before the getter. \
                                            */
#define FVM_INVOKE_MEMBER_ARGUMENT_BASE 4U /* [GC:] Copied arguments follow fixed roots. */
#endif
