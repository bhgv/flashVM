/* FlashVM: retained inputs for scoped named construction. */
#ifndef FVM_NEW_OBJECT_CONSTANTS_H
#define FVM_NEW_OBJECT_CONSTANTS_H      /* Private construction-root layout guard. */
#define FVM_NEW_OBJECT_SCOPE_ROOT 0U    /* [GC:] Managed lexical scope chain. */
#define FVM_NEW_OBJECT_TARGET_ROOT 1U   /* [GC:] Target used by ordinary lookup. */
#define FVM_NEW_OBJECT_THIS_ROOT 2U     /* [GC:] Independent implicit this binding. */
#define FVM_NEW_OBJECT_NAME_ROOT 3U     /* [GC:] Already-coerced counted constructor name. */
#define FVM_NEW_OBJECT_FUNCTION_ROOT 4U /* [GC:] Resolved constructor through callbacks. */
#define FVM_NEW_OBJECT_ARGUMENT_BASE 5U /* [GC:] Ordered argument copies follow fixed roots. */
#endif
