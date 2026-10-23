/* FlashVM: ActionScript lineStyle argument positions and supported version policy. */
#ifndef FVM_STANDARD_LINE_STYLE_CONSTANTS_H
#define FVM_STANDARD_LINE_STYLE_CONSTANTS_H /* Guard script line-style constants. */
#define FVM_LINE_STYLE_WIDTH 0U /* Thickness; omitted/undefined disables strokes. */
#define FVM_LINE_STYLE_RGB 1U /* Optional packed RGB color. */
#define FVM_LINE_STYLE_ALPHA 2U /* Optional alpha percentage. */
#define FVM_LINE_STYLE_HINTING 3U /* SWF8 pixel-hinting Boolean. */
#define FVM_LINE_STYLE_SCALE 4U /* SWF8 scale-mode string. */
#define FVM_LINE_STYLE_CAP 5U /* SWF8 symmetric cap string. */
#define FVM_LINE_STYLE_JOIN 6U /* SWF8 join string. */
#define FVM_LINE_STYLE_EXTRA_MIN_VERSION 8U /* Additional arguments begin in Flash Player 8. */
#define FVM_LINE_STYLE_MAX_WIDTH 255.0 /* Finite script widths clamp to the Flash range. */
#endif
