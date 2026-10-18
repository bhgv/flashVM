/* FlashVM: retained operands for scope-aware Delete2 reference deletion. */
#ifndef FVM_DELETE_REFERENCE_CONSTANTS_H
#define FVM_DELETE_REFERENCE_CONSTANTS_H    /* Private reference-root layout guard. */
#define FVM_DELETE_REFERENCE_SCOPE_ROOT 0U  /* [GC:] Original lexical chain. */
#define FVM_DELETE_REFERENCE_TARGET_ROOT 1U /* [GC:] Selected fallback target. */
#define FVM_DELETE_REFERENCE_THIS_ROOT 2U   /* [GC:] Explicit implicit-this receiver. */
#define FVM_DELETE_REFERENCE_NAME_ROOT 3U   /* [GC:] Coercible input name. */
#define FVM_DELETE_REFERENCE_TEXT_ROOT 4U   /* [GC:] Complete converted name. */
#define FVM_DELETE_REFERENCE_PREFIX_ROOT 5U /* [GC:] Variable reference identifying the owner. */
#define FVM_DELETE_REFERENCE_BASE_ROOT 6U   /* [GC:] Resolved owner value across final deletion. */
#define FVM_DELETE_REFERENCE_MEMBER_ROOT 7U /* [GC:] Final property to delete without reading. */
#define FVM_DELETE_REFERENCE_PATH_ROOT 8U   /* [GC:] Prefix of the variable reference. */
#define FVM_DELETE_REFERENCE_ROOT_COUNT 9U  /* [GC:] All input and intermediate root slots. */
#endif
