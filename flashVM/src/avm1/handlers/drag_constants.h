/* FlashVM: native drag preparation layout, separate from encoded Action operands. */
#ifndef FVM_DRAG_CONSTANTS_H
#define FVM_DRAG_CONSTANTS_H                /* Native drag layout guard. */
#define FVM_DRAG_BOUND_COMPONENT_COUNT 4U   /* Constraint rectangle has four scalar limits. */
#define FVM_DRAG_CLIP_ROOT 0U               /* [GC:] Original drag target through matrix callbacks. */
#define FVM_DRAG_PARENT_ROOT 1U             /* [GC:] Original physical parent through callback mutation. */
#define FVM_DRAG_UPDATE_ROOT_COUNT 2U       /* [GC:] Complete transform-update temporary layout. */
#define FVM_DRAG_VALUES_CLIP_ROOT 0U        /* [GC:] Native instance before deferred drag selection. */
#define FVM_DRAG_VALUES_CENTER_ROOT 1U      /* [GC:] Optional center value; objects project to true. */
#define FVM_DRAG_VALUES_FIRST_BOUND_ROOT 2U /* [GC:] First numeric bound, followed by trailing limits. */
#define FVM_DRAG_VALUES_ROOT_COUNT 6U       /* [GC:] Clip, center and all four possibly missing bounds. */
#endif
