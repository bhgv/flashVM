/* FlashVM: documented bounds shared by direct and queued host input. */
#ifndef FLASHVM_INPUT_LIMITS_H
#define FLASHVM_INPUT_LIMITS_H /* Public C89 input limits guard. */
/* Maximum simultaneous fingers accepted by rules, events and deferred dispatch. */
#define FVM_INPUT_MAX_FINGERS 16U
/* Maximum copied events pending in the per-context, owning-thread FIFO. */
#define FVM_INPUT_MAX_PENDING_EVENTS 1024U
#endif
