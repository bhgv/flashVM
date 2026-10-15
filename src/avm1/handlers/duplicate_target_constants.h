/* FlashVM: explicit target-duplication request roots. */
#ifndef FVM_DUPLICATE_TARGET_CONSTANTS_H
#define FVM_DUPLICATE_TARGET_CONSTANTS_H /* Request root layout guard. */
#define FVM_DUPLICATE_TARGET_START_ROOT 0U /* [GC:] Original native search base. */
#define FVM_DUPLICATE_TARGET_SOURCE_ROOT 1U /* [GC:] Source value through earlier coercions. */
#define FVM_DUPLICATE_TARGET_NAME_ROOT 2U /* [GC:] Original name through depth conversion. */
#define FVM_DUPLICATE_TARGET_DEPTH_ROOT 3U /* [GC:] Depth input, then signed integer result. */
#define FVM_DUPLICATE_TARGET_TEXT_ROOT 4U /* [GC:] Converted counted name. */
#define FVM_DUPLICATE_TARGET_CLIP_ROOT 5U /* [GC:] Resolved native source. */
#define FVM_DUPLICATE_TARGET_RESULT_ROOT 6U /* [GC:] Result before caller publication. */
#define FVM_DUPLICATE_TARGET_ROOT_COUNT 7U /* [GC:] Complete clone request root layout. */
#endif
