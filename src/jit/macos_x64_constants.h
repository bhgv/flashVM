/* FlashVM: Darwin x86-64 frame-chain and stack-alignment instructions. */
#ifndef FVM_JIT_MACOS_X64_CONSTANTS_H
#define FVM_JIT_MACOS_X64_CONSTANTS_H /* Guard Darwin-specific frame encodings. */
#define FVM_JIT_MAC64_PUSH_RBP 0x55U /* Preserve the parent frame pointer. */
#define FVM_JIT_MAC64_POP_RBP 0x5dU /* Restore the parent frame pointer on every exit. */
#define FVM_JIT_MAC64_REX_W 0x48U /* Select 64-bit frame and stack operations. */
#define FVM_JIT_MAC64_MOV_RBP 0x89U /* Register move for the frame link. */
#define FVM_JIT_MAC64_RBP_RSP 0xe5U /* mov rbp,rsp ModRM selector. */
#define FVM_JIT_MAC64_GROUP_IMM8 0x83U /* Group-1 stack adjustment with immediate byte. */
#define FVM_JIT_MAC64_SUB_RSP 0xecU /* Allocate the alignment slot. */
#define FVM_JIT_MAC64_ADD_RSP 0xc4U /* Release the alignment slot without changing EAX. */
#define FVM_JIT_MAC64_ALIGN_BYTES 8U /* Restore 16-byte call alignment after saved RBP and r12. */
#endif
