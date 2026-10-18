/* FlashVM: fixed Darwin ARM64 frame instructions, independent of code publication policy. */
#ifndef FVM_JIT_MACOS_ARM64_CONSTANTS_H
#define FVM_JIT_MACOS_ARM64_CONSTANTS_H /* Guard the Darwin-specific frame encodings. */
/* STP X19,X20,[SP,#-32]! reserves an aligned frame and preserves invocation registers. */
#define FVM_JIT_MAC_A64_SAVE_STATE 0xa9be53f3UL
/* STP X29,X30,[SP,#16] stores the parent frame and caller return address. */
#define FVM_JIT_MAC_A64_SAVE_FRAME 0xa9017bfdUL
/* ADD X29,SP,#16 links the frame record before any helper is called. */
#define FVM_JIT_MAC_A64_LINK_FRAME 0x910043fdUL
/* LDP X29,X30,[SP,#16] restores the parent frame and caller return address. */
#define FVM_JIT_MAC_A64_RESTORE_FRAME 0xa9417bfdUL
/* LDP X19,X20,[SP],#32 restores preserved state and releases the frame. */
#define FVM_JIT_MAC_A64_RESTORE_STATE 0xa8c253f3UL
#endif
