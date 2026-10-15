/* FlashVM: temporary roots for native local transform copying. */
#ifndef FVM_CLIP_COPY_TRANSFORM_CONSTANTS_H
#define FVM_CLIP_COPY_TRANSFORM_CONSTANTS_H /* Private copy-service layout guard. */
#define FVM_CLIP_COPY_DESTINATION_ROOT 0U   /* [GC:] Destination retained across source getters. */
#define FVM_CLIP_COPY_SOURCE_ROOT 1U        /* [GC:] Source retained across geometry callbacks. */
#define FVM_CLIP_COPY_ROOT_COUNT 2U         /* [GC:] Complete native copy-service root layout. */
#endif
