/* FlashVM: retained values for lexical retries of counted object paths. */
#ifndef FVM_SCOPE_TARGET_CONSTANTS_H
#define FVM_SCOPE_TARGET_CONSTANTS_H     /* Private scoped-target root-layout guard. */
#define FVM_SCOPE_TARGET_SCOPE_ROOT 0U   /* [GC:] Original managed lexical chain. */
#define FVM_SCOPE_TARGET_TARGET_ROOT 1U  /* [GC:] Selected fallback timeline object. */
#define FVM_SCOPE_TARGET_PATH_ROOT 2U    /* [GC:] Complete counted path through getters. */
#define FVM_SCOPE_TARGET_CURRENT_ROOT 3U /* [GC:] Current scope survives chain detachment. */
#define FVM_SCOPE_TARGET_DISPLAY_ROOT 4U /* [GC:] Shared absolute-path root for every retry. */
#define FVM_SCOPE_TARGET_THIS_ROOT                                                                                     \
    5U                                   /* [GC:] Explicit this independent of the selected target.  \
                                        */
#define FVM_SCOPE_TARGET_ROOT_COUNT 6U   /* [GC:] Number of retained input/intermediate values. */
#endif
