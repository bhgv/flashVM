/* FlashVM: roots for final-member-aware path lookup across lexical scopes. */
#ifndef FVM_SCOPE_MEMBER_CONSTANTS_H
#define FVM_SCOPE_MEMBER_CONSTANTS_H     /* Private scoped-member root-layout guard. */
#define FVM_SCOPE_MEMBER_SCOPE_ROOT 0U   /* [GC:] Original activation/With chain. */
#define FVM_SCOPE_MEMBER_TARGET_ROOT 1U  /* [GC:] Selected fallback target. */
#define FVM_SCOPE_MEMBER_THIS_ROOT 2U    /* [GC:] Explicit receiver independent of target. */
#define FVM_SCOPE_MEMBER_PATH_ROOT 3U    /* [GC:] Counted object-prefix path. */
#define FVM_SCOPE_MEMBER_NAME_ROOT 4U    /* [GC:] Counted final property name. */
#define FVM_SCOPE_MEMBER_CURRENT_ROOT 5U /* [GC:] Current scope survives chain detachment. */
#define FVM_SCOPE_MEMBER_DISPLAY_ROOT 6U /* [GC:] Shared absolute-path display root. */
#define FVM_SCOPE_MEMBER_ROOT_COUNT 7U   /* [GC:] All retained inputs and traversal state. */
#endif
