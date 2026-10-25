/* FlashVM: retained inputs and path slices for variable assignment. */
#ifndef FVM_VARIABLE_SET_CONSTANTS_H
#define FVM_VARIABLE_SET_CONSTANTS_H    /* Private variable-write root-layout guard. */
#define FVM_VARIABLE_SET_SCOPE_ROOT 0U  /* [GC:] Managed activation/With chain. */
#define FVM_VARIABLE_SET_TARGET_ROOT 1U /* [GC:] Explicit fallback destination. */
#define FVM_VARIABLE_SET_THIS_ROOT 2U   /* [GC:] Independent receiver for this path components. */
#define FVM_VARIABLE_SET_NAME_ROOT 3U   /* [GC:] Counted variable reference. */
#define FVM_VARIABLE_SET_VALUE_ROOT 4U  /* [GC:] Assigned value retained through setters. */
#define FVM_VARIABLE_SET_PREFIX_ROOT 5U /* [GC:] Qualified owner-path slice. */
#define FVM_VARIABLE_SET_MEMBER_ROOT 6U /* [GC:] Counted final property name. */
#define FVM_VARIABLE_SET_OWNER_ROOT 7U  /* [GC:] Resolved destination after path getters. */
#define FVM_VARIABLE_SET_TEXT_ROOT 8U   /* [GC:] Converted counted name, separate from caller input. */
#define FVM_VARIABLE_SET_ROOT_COUNT 9U  /* [GC:] Number of retained inputs and intermediates. */
#endif
