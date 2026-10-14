/* FlashVM: retained values for the qualified GetVariable path phase. */
#ifndef FVM_VARIABLE_PATH_CONSTANTS_H
#define FVM_VARIABLE_PATH_CONSTANTS_H    /* Private variable-path root-layout guard. */
#define FVM_VARIABLE_PATH_SCOPE_ROOT 0U  /* [GC:] Original managed lexical scope chain. */
#define FVM_VARIABLE_PATH_TARGET_ROOT 1U /* [GC:] Selected fallback target. */
#define FVM_VARIABLE_PATH_THIS_ROOT 2U   /* [GC:] Receiver used by explicit this components. */
#define FVM_VARIABLE_PATH_NAME_ROOT 3U   /* [GC:] Complete counted input name. */
#define FVM_VARIABLE_PATH_PREFIX_ROOT 4U /* [GC:] Object-prefix slice. */
#define FVM_VARIABLE_PATH_MEMBER_ROOT 5U /* [GC:] Final variable-name slice. */
#define FVM_VARIABLE_PATH_RESULT_ROOT 6U /* [GC:] Temporary result before publishing outputs. */
#define FVM_VARIABLE_PATH_ROOT_COUNT 7U  /* [GC:] Number of retained inputs and slices. */
#endif
