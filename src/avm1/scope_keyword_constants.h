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
/* Private semantic path kinds; independent of bytecode and serialized callback IDs. */
#define FVM_SCOPE_PATH_NONE 0U                  /* No intrinsic field with this spelling/version. */
#define FVM_SCOPE_PATH_ROOT 1U                  /* Native root reference, loaded SWF5+. */
#define FVM_SCOPE_PATH_PARENT 2U                /* Native parent reference, loaded SWF5+. */
#define FVM_SCOPE_PATH_GLOBAL 3U                /* Context namespace reference, loaded SWF6+. */
#define FVM_SCOPE_CASE_SOURCE_COLUMN 0U /* Original unit in pinned Flash lowercase map. */
#define FVM_SCOPE_CASE_RESULT_COLUMN                                                       \
    1U /* Lowercase unit in pinned Flash lowercase map.                                            \
        */
/* Two counted names participate in allocation-free folded comparison. */
#define FVM_SCOPE_NAME_OPERAND_COUNT 2U
#endif
