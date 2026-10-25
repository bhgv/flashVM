/* FlashVM: rooted environment, name, callee and arguments for named function calls. */
#ifndef FVM_CALL_FUNCTION_CONSTANTS_H
#define FVM_CALL_FUNCTION_CONSTANTS_H      /* Private named-call root-layout guard. */
#define FVM_CALL_FUNCTION_SCOPE_ROOT 0U    /* [GC:] Managed lexical scopes retained during lookup. */
#define FVM_CALL_FUNCTION_TARGET_ROOT 1U   /* [GC:] Explicit variable-path target. */
#define FVM_CALL_FUNCTION_THIS_ROOT 2U     /* [GC:] Independent implicit-this object. */
#define FVM_CALL_FUNCTION_DEFAULT_ROOT 3U  /* [GC:] Default callable receiver. */
#define FVM_CALL_FUNCTION_NAME_ROOT 4U     /* [GC:] Already-coerced counted function name. */
#define FVM_CALL_FUNCTION_VALUE_ROOT 5U    /* [GC:] Resolved callee retained through invocation. */
#define FVM_CALL_FUNCTION_RECEIVER_ROOT 6U /* [GC:] Actual receiver returned by binding lookup. */
#define FVM_CALL_FUNCTION_SOURCE_ROOT 7U   /* [GC:] Defining prototype for superclass dispatch. */
#define FVM_CALL_FUNCTION_ARGUMENT_BASE 8U /* [GC:] Ordered arguments follow fixed roots. */
#endif
