/* FlashVM: retained operands for primitive-to-object conversion. */
#ifndef FVM_TO_OBJECT_CONSTANTS_H
#define FVM_TO_OBJECT_CONSTANTS_H    /* Object conversion constants guard. */
#define FVM_TO_OBJECT_INPUT_ROOT 0U  /* [GC:] Original primitive while constructor getters run. */
#define FVM_TO_OBJECT_NAME_ROOT 1U   /* [GC:] Counted global constructor name. */
#define FVM_TO_OBJECT_RESULT_ROOT 2U /* [GC:] Constructed wrapper or undefined. */
#define FVM_TO_OBJECT_ROOT_COUNT 3U  /* [GC:] Input and intermediate root slots. */
#define FVM_TO_OBJECT_ARGUMENT_COUNT 1U /* Boxing constructors receive the original primitive. */
#endif
