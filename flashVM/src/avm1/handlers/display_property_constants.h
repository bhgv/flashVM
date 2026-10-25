/* FlashVM: retained operands and conversion bounds for display-property operations. */
#ifndef FVM_DISPLAY_PROPERTY_CONSTANTS_H
#define FVM_DISPLAY_PROPERTY_CONSTANTS_H          /* Private display-property root layout guard. */
#define FVM_DISPLAY_PROPERTY_TARGET_TEXT_ROOT 0U  /* [GC:] Converted target path. */
#define FVM_DISPLAY_PROPERTY_VALUE_ROOT                                                                                \
    1U                                            /* [GC:] Converted value retained through publication.  \
                                            */
#define FVM_DISPLAY_PROPERTY_CLIP_ROOT 2U         /* [GC:] Resolved clip survives value coercion. */
#define FVM_DISPLAY_PROPERTY_INPUT_TARGET_ROOT 3U /* [GC:] Original target through index coercion. */
#define FVM_DISPLAY_PROPERTY_INPUT_INDEX_ROOT 4U  /* [GC:] Original index through callbacks. */
#define FVM_DISPLAY_PROPERTY_INPUT_VALUE_ROOT 5U  /* [GC:] Original value through target resolution. */
#define FVM_DISPLAY_PROPERTY_ROOT_COUNT 6U        /* [GC:] All original and converted snapshots. */
/* Exclusive lower bound permits negative fractions that truncate to property zero. */
#define FVM_DISPLAY_PROPERTY_INDEX_MIN_NUMBER (-1.0)
#endif
