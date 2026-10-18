/* FlashVM: bounded display-path traversal and temporary collector-root layout. */
#ifndef FVM_MOVIECLIP_PATH_CONSTANTS_H
#define FVM_MOVIECLIP_PATH_CONSTANTS_H      /* Private display-path constant guard. */
#define FVM_MOVIECLIP_PATH_MAX_VISITS 1024U /* Bound parent, prototype and component walks. */
#define FVM_MOVIECLIP_PATH_START_ROOT 0U    /* [GC:] Original base survives detached path edges. */
#define FVM_MOVIECLIP_PATH_DISPLAY_ROOT 1U  /* [GC:] Display root for absolute paths. */
#define FVM_MOVIECLIP_PATH_TEXT_ROOT 2U     /* [GC:] Counted input survives getter callbacks. */
#define FVM_MOVIECLIP_PATH_CURRENT_ROOT 3U  /* [GC:] Current object survives a getter call. */
#define FVM_MOVIECLIP_PATH_THIS_ROOT 4U     /* [GC:] Explicit this object for variable paths. */
#define FVM_MOVIECLIP_PATH_ROOT_COUNT 5U    /* [GC:] Number of resolver root slots. */
#define FVM_MOVIECLIP_PATH_STATIC_ART                                                              \
    1 /* Private static-art marker resolves to its parent clip.                                    \
       */
#define FVM_MOVIECLIP_PATH_LEVEL_PREFIX_BYTES 40U /* Decimal signed level, _level prefix and NUL.  \
                                                   */
#endif
