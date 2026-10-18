/* FlashVM: retained inputs and legacy this-name mapping for plain variable reads. */
#ifndef FVM_VARIABLE_LOOKUP_CONSTANTS_H
#define FVM_VARIABLE_LOOKUP_CONSTANTS_H     /* Private plain-lookup layout guard. */
#define FVM_VARIABLE_LOOKUP_SCOPE_ROOT 0U   /* [GC:] Original managed lexical chain. */
#define FVM_VARIABLE_LOOKUP_TARGET_ROOT 1U  /* [GC:] Selected target for property fallback. */
#define FVM_VARIABLE_LOOKUP_THIS_ROOT 2U    /* [GC:] Explicit this value. */
#define FVM_VARIABLE_LOOKUP_DEFAULT_ROOT 3U /* [GC:] Default callable receiver. */
#define FVM_VARIABLE_LOOKUP_NAME_ROOT 4U    /* [GC:] Original counted variable name. */
#define FVM_VARIABLE_LOOKUP_RESULT_ROOT 5U  /* [GC:] Result retained before output publication. */
#define FVM_VARIABLE_LOOKUP_ROOT_COUNT 6U   /* [GC:] Number of retained operands/results. */
#endif
