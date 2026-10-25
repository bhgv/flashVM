/* FlashVM: temporary roots for explicit native display-target resolution. */
#ifndef FVM_RESOLVE_CLIP_CONSTANTS_H
#define FVM_RESOLVE_CLIP_CONSTANTS_H   /* Explicit display-target layout guard. */
#define FVM_RESOLVE_CLIP_START_ROOT 0U /* [GC:] Original native path start. */
#define FVM_RESOLVE_CLIP_INPUT_ROOT 1U /* [GC:] Input retained during coercion. */
#define FVM_RESOLVE_CLIP_PATH_ROOT 2U  /* [GC:] Counted path retained during lookup. */
#define FVM_RESOLVE_CLIP_ROOT_COUNT 3U /* [GC:] Complete resolver temporary layout. */
#endif
