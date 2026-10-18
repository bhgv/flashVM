/* FlashVM: input and intermediate roots for complete GetVariable semantics. */
#ifndef FVM_GET_VARIABLE_CONSTANTS_H
#define FVM_GET_VARIABLE_CONSTANTS_H    /* GetVariable root-layout guard. */
#define FVM_GET_VARIABLE_SCOPE_ROOT 0U  /* [GC:] Lexical scope retained through name conversion. */
#define FVM_GET_VARIABLE_TARGET_ROOT 1U /* [GC:] Explicit target retained through callbacks. */
#define FVM_GET_VARIABLE_THIS_ROOT 2U   /* [GC:] Implicit this binding independent of target. */
#define FVM_GET_VARIABLE_NAME_ROOT 3U   /* [GC:] Original potentially coercible name. */
#define FVM_GET_VARIABLE_TEXT_ROOT 4U   /* [GC:] Counted name after conversion. */
#define FVM_GET_VARIABLE_VALUE_ROOT 5U  /* [GC:] Read value before SWF4 action filtering. */
#define FVM_GET_VARIABLE_ROOT_COUNT 6U  /* [GC:] All environment/name/result snapshots. */
#endif
