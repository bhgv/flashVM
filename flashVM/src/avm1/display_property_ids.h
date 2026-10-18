/* FlashVM: numeric AVM1 display-property indices for GetProperty/SetProperty. */
#ifndef FVM_AVM1_DISPLAY_PROPERTY_IDS_H
#define FVM_AVM1_DISPLAY_PROPERTY_IDS_H /* Header guard. */
/* Horizontal position; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_X 0U
/* Vertical position; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_Y 1U
/* Horizontal scale percentage; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_XSCALE 2U
/* Vertical scale percentage; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_YSCALE 3U
/* Current timeline frame; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_CURRENT_FRAME 4U
/* Total timeline frame count; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_TOTAL_FRAMES 5U
/* Alpha percentage; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_ALPHA 6U
/* Visibility flag; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_VISIBLE 7U
/* Display width; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_WIDTH 8U
/* Display height; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_HEIGHT 9U
/* Rotation angle; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_ROTATION 10U
/* Display target path; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_TARGET 11U
/* Loaded timeline frame count; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_FRAMES_LOADED 12U
/* Instance name; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_NAME 13U
/* Current drop target; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_DROP_TARGET 14U
/* Movie URL; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_URL 15U
/* Legacy rendering quality flag; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_HIGH_QUALITY 16U
/* Focus rectangle policy; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_FOCUS_RECT 17U
/* Sound buffer duration; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_SOUND_BUFFER_TIME 18U
/* Rendering quality label; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_QUALITY 19U
/* Mouse X relative to the clip; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_XMOUSE 20U
/* Mouse Y relative to the clip; availability/writability is checked by the display service. */
#define FVM_AVM1_PROPERTY_YMOUSE 21U
/* Upper bound (exclusive) for an encoded display-property index. */
#define FVM_AVM1_PROPERTY_COUNT 22U
#endif
