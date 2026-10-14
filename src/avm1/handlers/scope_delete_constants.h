/* FlashVM: root slots for deletion of a binding in retained lexical scopes. */
#ifndef FVM_SCOPE_DELETE_CONSTANTS_H
#define FVM_SCOPE_DELETE_CONSTANTS_H    /* Private root-layout guard. */
#define FVM_SCOPE_DELETE_SCOPE_ROOT 0U  /* Innermost managed activation/With scope. */
#define FVM_SCOPE_DELETE_TARGET_ROOT 1U /* Selected fallback object retained through coercion. */
#define FVM_SCOPE_DELETE_NAME_ROOT 2U   /* Incoming coercible binding name. */
#define FVM_SCOPE_DELETE_TEXT_ROOT 3U   /* Converted counted name retained through deletion. */
#define FVM_SCOPE_DELETE_ROOT_COUNT 4U  /* All source/intermediate roots for this service. */
#endif
