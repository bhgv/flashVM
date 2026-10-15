/* FlashVM: native drag preparation layout, separate from encoded Action operands. */
#ifndef FVM_DRAG_CONSTANTS_H
#define FVM_DRAG_CONSTANTS_H /* Native drag layout guard. */
#define FVM_DRAG_BOUND_COMPONENT_COUNT 4U /* Constraint rectangle has four scalar limits. */
#define FVM_DRAG_CLIP_ROOT 0U /* [GC:] Original drag target through matrix callbacks. */
#define FVM_DRAG_PARENT_ROOT 1U /* [GC:] Original physical parent through callback mutation. */
#define FVM_DRAG_UPDATE_ROOT_COUNT 2U /* [GC:] Complete transform-update temporary layout. */
#endif
