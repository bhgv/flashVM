/* FlashVM: legacy loaded-frame bounds, compatibility arithmetic and temporary roots. */
#ifndef FVM_FRAME_LOADED_CONSTANTS_H
#define FVM_FRAME_LOADED_CONSTANTS_H /* Frame availability service guard. */
#define FVM_FRAME_LOADED_DIRECT_LIMIT                                                              \
    16000L /* Largest direct frame request accepted by the player rule. */
#define FVM_FRAME_LOADED_VALUE_LIMIT                                                               \
    16001L /* Value requests retain the historical extra accepted frame. */
#define FVM_FRAME_LOADED_VALUE_SLOP 1.0 /* Value requests compare against frames-loaded plus one.  \
                                         */
#define FVM_FRAME_LOADED_U16_MODULUS                                                               \
    65536L /* Reinterpret a signed value request as an unsigned frame. */
#define FVM_FRAME_LOADED_INPUT_ROOT 0U /* [GC:] Original frame operand through string coercion. */
#define FVM_FRAME_LOADED_TEXT_ROOT                                                                 \
    1U /* [GC:] Converted counted text while parsing its numeric syntax. */
#define FVM_FRAME_LOADED_ROOT_COUNT 2U /* [GC:] Original and converted operands. */
#endif
