/* Native text tracking limits, independent of font size and public option ABI. */
#ifndef FVM_TEXT_LAYOUT_CONSTANTS_H
#define FVM_TEXT_LAYOUT_CONSTANTS_H /* Scope native layout admission constants. */
#define FVM_TEXT_LAYOUT_MAX_SPACING 4096.0 /* Maximum absolute extra pixels after a supported character. */
#define FVM_TEXT_LAYOUT_MAX_EXTENT 1000000.0 /* Maximum logical text extent admitted by native layout. */
#define FVM_TEXT_LAYOUT_RECT_COMMANDS 5U /* Four corner visits plus a closing path command. */
#endif
