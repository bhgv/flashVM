/* FlashVM: transport-safe source identity encoding for root sound notifications. */
#ifndef FVM_MOVIECLIP_SOUND_CONSTANTS_H
#define FVM_MOVIECLIP_SOUND_CONSTANTS_H     /* Guard scoped source-comparison constants. */
#define FVM_MOVIECLIP_SOUND_HEX_PER_BYTE 2U /* ASCII characters per original SWF byte. */
#define FVM_MOVIECLIP_SOUND_NIBBLE_SHIFT 4U /* Bits in one encoded nibble. */
#define FVM_MOVIECLIP_SOUND_NIBBLE_MASK 15U /* Select one low hexadecimal digit. */
/* Maximum native snapshot entries at one root sound-bearing frame. */
#define FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS 65536UL
#endif
