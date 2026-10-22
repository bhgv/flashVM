/* Native text tracking limits, independent of font size and public option ABI. */
#ifndef FVM_TEXT_LAYOUT_CONSTANTS_H
#define FVM_TEXT_LAYOUT_CONSTANTS_H /* Scope native layout admission constants. */
#define FVM_TEXT_LAYOUT_JUSTIFY_SPACE 0x20U /* Only ASCII interword spaces are expanded. */
#define FVM_TEXT_LAYOUT_MAX_SPACING 4096.0 /* Maximum absolute extra pixels after a supported character. */
#define FVM_TEXT_LAYOUT_MAX_EXTENT 1000000.0 /* Maximum logical text extent admitted by native layout. */
#define FVM_TEXT_LAYOUT_RECT_COMMANDS 5U /* Four corner visits plus a closing path command. */
#define FVM_TEXT_LAYOUT_LINE_POINTS 1U /* Endpoints in a straight path command. */
#define FVM_TEXT_LAYOUT_QUADRATIC_POINTS 2U /* Control and endpoint of a quadratic. */
#define FVM_TEXT_LAYOUT_CUBIC_POINTS 3U /* Two controls and endpoint of a cubic. */
#define FVM_TEXT_LAYOUT_WINDING_NONE 0 /* No nondegenerate outline supplied a direction. */
#define FVM_TEXT_LAYOUT_WINDING_NEGATIVE (-1) /* Negative signed control-polygon area. */
#define FVM_TEXT_LAYOUT_WINDING_POSITIVE 1 /* Positive signed control-polygon area. */
#endif
