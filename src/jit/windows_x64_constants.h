/* FlashVM: Microsoft x64 call frames and matching Windows unwind representation. */
#ifndef FVM_JIT_WINDOWS_X64_CONSTANTS_H
#define FVM_JIT_WINDOWS_X64_CONSTANTS_H /* Guard Windows ABI instructions and frame layout. */
#define FVM_JIT_WIN64_SHADOW_BYTES 32U /* Four mandatory eight-byte helper home slots. */
#define FVM_JIT_WIN64_REX_W 0x48U /* Select 64-bit stack arithmetic. */
#define FVM_JIT_WIN64_GROUP_IMM8 0x83U /* Group-1 stack adjustment with immediate byte. */
#define FVM_JIT_WIN64_SUB_RSP 0xecU /* Subtract the immediate from RSP. */
#define FVM_JIT_WIN64_ADD_RSP 0xc4U /* Restore RSP by adding the shadow-space size. */
#define FVM_JIT_WIN64_SAVE_RCX 0xccU /* mov r12,rcx register selector after 49 89. */
#define FVM_JIT_WIN64_ARG_RCX 0xe1U /* mov rcx,r12 register selector after 4c 89. */
#define FVM_JIT_WIN64_OPERAND_EDX 0xbaU /* mov edx,imm32; second integer helper argument. */
#define FVM_JIT_WIN64_PUSH_END 6U /* End of ENDBR64 plus push r12. */
#define FVM_JIT_WIN64_ALLOC_END 10U /* End of sub rsp,32 in the native prologue. */
#define FVM_JIT_WIN64_PROLOGUE_BYTES 13U /* Complete fixed prologue, including mov r12,rcx. */
#define FVM_JIT_WIN64_UNWIND_VERSION 1U /* UNWIND_INFO version one, no exception handler. */
#define FVM_JIT_WIN64_UNWIND_CODES 2U /* One allocation record and one saved-register record. */
#define FVM_JIT_WIN64_UNWIND_ALLOC 0x32U /* UWOP_ALLOC_SMALL with 32-byte allocation. */
#define FVM_JIT_WIN64_UNWIND_PUSH 0xc0U /* UWOP_PUSH_NONVOL for r12. */
#define FVM_JIT_WIN64_UNWIND_FRAME 0U /* No frame-pointer register or scaled frame offset. */
#define FVM_JIT_WIN64_UNWIND_ALIGNMENT 4U /* DWORD alignment required by Windows unwind RVAs. */
#define FVM_JIT_WIN64_FUNCTION_ENTRIES 1U /* One native function range per independently owned mapping. */
#define FVM_JIT_WIN64_UNWIND_BYTES 8U /* Four-byte header plus two two-byte unwind codes. */
#endif
