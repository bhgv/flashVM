/* FlashVM: bounds for the shared macOS ARM64 executable-memory arena. */
#ifndef FVM_MAC_CODE_ARM64_CONSTANTS_H
#define FVM_MAC_CODE_ARM64_CONSTANTS_H /* Guard allocation bounds. */
/* One 64-MiB process-lifetime region supports many separately owned code slots. */
#define FVM_MAC_JIT_ARENA_BYTES (64UL * 1024UL * 1024UL)
/* Slots start on 16-byte boundaries; AArch64 instructions require only four. */
#define FVM_MAC_JIT_SLOT_ALIGNMENT 16UL
#endif
