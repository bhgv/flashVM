/* FlashVM: retained operands and coercion results for local declarations. */
#ifndef FVM_VARIABLE_DECLARE_CONSTANTS_H
#define FVM_VARIABLE_DECLARE_CONSTANTS_H        /* Private declaration-root guard. */
#define FVM_VARIABLE_DECLARE_SCOPE_ROOT 0U      /* [GC:] Current managed lexical chain. */
#define FVM_VARIABLE_DECLARE_ACTIVATION_ROOT 1U /* [GC:] Current local frame, when present. */
#define FVM_VARIABLE_DECLARE_TARGET_ROOT 2U     /* [GC:] Explicit timeline/global destination. */
#define FVM_VARIABLE_DECLARE_THIS_ROOT 3U       /* [GC:] Receiver for qualified top-level paths. */
#define FVM_VARIABLE_DECLARE_NAME_ROOT 4U       /* [GC:] Original coercible declaration name. */
#define FVM_VARIABLE_DECLARE_VALUE_ROOT 5U      /* [GC:] Initializer or undefined value. */
#define FVM_VARIABLE_DECLARE_TEXT_ROOT 6U       /* [GC:] Converted counted name. */
#define FVM_VARIABLE_DECLARE_EXISTING_ROOT 7U   /* [GC:] Value read by qualified DeclareLocal2. */
#define FVM_VARIABLE_DECLARE_ROOT_COUNT 8U      /* [GC:] Number of input/result roots. */
#endif
