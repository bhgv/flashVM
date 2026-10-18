/* FlashVM: rooted inputs and boxed display objects for TargetPath. */
#ifndef FVM_TARGET_PATH_CONSTANTS_H
#define FVM_TARGET_PATH_CONSTANTS_H /* TargetPath service guard. */
#define FVM_TARGET_PATH_INPUT_ROOT                                                                 \
    0U /* [GC:] Original value during primitive constructor dispatch. */
#define FVM_TARGET_PATH_BOXED_ROOT                                                                 \
    1U /* [GC:] Prepared object during native display-path copying. */
#define FVM_TARGET_PATH_ROOT_COUNT 2U /* [GC:] Original and prepared operands. */
#endif
