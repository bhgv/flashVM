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
#define FVM_VARIABLE_THIS_NAME                                                                     \
    "this" /* Implicit receiver keyword, not an ordinary path component. */
#define FVM_VARIABLE_THIS_UNITS                                                                    \
    (sizeof(FVM_VARIABLE_THIS_NAME) - 1U) /* UCS2 length of the keyword. */
/* Namespace keyword exists as an implicit fallback only in loaded SWF6+. */
#define FVM_VARIABLE_GLOBAL_NAME "_global"
#define FVM_VARIABLE_GLOBAL_UNITS                                                                  \
    (sizeof(FVM_VARIABLE_GLOBAL_NAME) - 1U) /* Counted UCS2 keyword length. */
#define FVM_VARIABLE_CASE_SOURCE_COLUMN                                                            \
    0U /* Original UCS2 unit in the pinned Flash case table.                                       \
        */
#define FVM_VARIABLE_CASE_RESULT_COLUMN                                                            \
    1U /* Lowercase UCS2 unit in the pinned Flash case table.                                      \
        */
#endif
