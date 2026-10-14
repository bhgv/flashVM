/* FlashVM: counted member-path syntax and retained traversal values. */
#ifndef FVM_DELETE_PATH_CONSTANTS_H
#define FVM_DELETE_PATH_CONSTANTS_H /* Private path-layout guard. */
#define FVM_DELETE_PATH_DOT '.'     /* Member-path separator accepted by Delete2. */
#define FVM_DELETE_PATH_COLON ':'   /* Legacy member-path separator accepted by Delete2. */
#define FVM_DELETE_PATH_SLASH '/'   /* Display-target slash syntax requires a separate resolver. */
#define FVM_DELETE_PATH_CURRENT_ROOT 0U /* Current object retained during a getter call. */
#define FVM_DELETE_PATH_TEXT_ROOT 1U    /* Complete counted path, including any embedded NUL. */
#define FVM_DELETE_PATH_NEXT_ROOT 2U    /* Getter result retained before the next traversal step. */
#define FVM_DELETE_PATH_MEMBER_ROOT 3U  /* Counted final member slice retained through deletion. */
#define FVM_DELETE_PATH_BASE_ROOT                                                                  \
    4U /* Original base survives getters which detach the traversed graph. */
#define FVM_DELETE_PATH_ROOT_COUNT 5U /* Traversal and final-deletion temporary roots. */
/* Reject longer member paths before invoking any traversal getter. */
#define FVM_DELETE_PATH_MAX_SEGMENTS 1024U
#endif
