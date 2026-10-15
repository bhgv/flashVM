/* FlashVM: native MovieClip drag argument layout, distinct from Action stack operands. */
#ifndef FVM_MOVIECLIP_DRAG_CONSTANTS_H
#define FVM_MOVIECLIP_DRAG_CONSTANTS_H /* Native drag operand layout guard. */
#define FVM_MOVIECLIP_DRAG_CENTER_INDEX 0U /* Optional lock-center value. */
#define FVM_MOVIECLIP_DRAG_FIRST_BOUND_INDEX 1U /* First parent-space constraint argument. */
#define FVM_MOVIECLIP_DRAG_BOUND_ARGUMENTS 4U /* At most xMin/yMin/xMax/yMax are used. */
#endif
