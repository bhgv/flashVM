/* FlashVM: counted implicit-scope keyword identities and case-table fields. */
#ifndef FVM_SCOPE_KEYWORD_CONSTANTS_H
#define FVM_SCOPE_KEYWORD_CONSTANTS_H      /* Shared scope keyword constants guard. */
#define FVM_SCOPE_KEYWORD_THIS_NAME "this" /* Explicit receiver keyword spelling. */
#define FVM_SCOPE_KEYWORD_THIS_UNITS (sizeof(FVM_SCOPE_KEYWORD_THIS_NAME) - 1U) /* UCS2 units. */
#define FVM_SCOPE_KEYWORD_GLOBAL_NAME "_global" /* Namespace fallback spelling, loaded SWF6+. */
#define FVM_SCOPE_KEYWORD_GLOBAL_UNITS                                                             \
    (sizeof(FVM_SCOPE_KEYWORD_GLOBAL_NAME) - 1U) /* UCS2 units. */
/* Root/parent intrinsic path fields begin in loaded SWF5. */
#define FVM_SCOPE_KEYWORD_ROOT_NAME "_root"
#define FVM_SCOPE_KEYWORD_ROOT_UNITS (sizeof(FVM_SCOPE_KEYWORD_ROOT_NAME) - 1U) /* UCS2 units. */
#define FVM_SCOPE_KEYWORD_PARENT_NAME "_parent"
#define FVM_SCOPE_KEYWORD_PARENT_UNITS                                                             \
    (sizeof(FVM_SCOPE_KEYWORD_PARENT_NAME) - 1U) /* UCS2 units. */
#define FVM_SCOPE_KEYWORD_CASE_SOURCE_COLUMN 0U  /* Original unit in pinned Flash lowercase map. */
#define FVM_SCOPE_KEYWORD_CASE_RESULT_COLUMN                                                       \
    1U /* Lowercase unit in pinned Flash lowercase map.                                            \
        */
#endif
