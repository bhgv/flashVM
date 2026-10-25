/* FlashVM: Windows x64 W^X code publication with registered fixed-frame unwind data. */
#include "../code_port.h"
#include "code_sdk.h"
#include "../../src/jit/constants.h"
#include "../../src/jit/windows_x64_constants.h"
#include <string.h>


/* Advertise only the Microsoft x64 ABI implemented by the corresponding encoder. */
int fvm_port_code_supported(void)
{
#if (defined(_M_X64) || defined(__x86_64__)) && !defined(_M_ARM64EC) && !defined(__arm64ec__)
    return sizeof(void *) == FVM_JIT_POINTER64_BYTES;
#else
    return 0;
#endif
}


/* Copy code/metadata while writable, seal RX, synchronize and register before publication. */
FVM_Result fvm_port_code_create(const void *bytes, size_t size, void **out, size_t *mapped)
{
#if (defined(_M_X64) || defined(__x86_64__)) && !defined(_M_ARM64EC) && !defined(__arm64ec__)
    static const unsigned char prologue[] = {FVM_JIT_ENDBR_0,
                                             FVM_JIT_ENDBR_1,
                                             FVM_JIT_ENDBR_2,
                                             FVM_JIT_ENDBR_3,
                                             FVM_JIT_PUSH_R12_0,
                                             FVM_JIT_PUSH_R12_1,
                                             FVM_JIT_WIN64_REX_W,
                                             FVM_JIT_WIN64_GROUP_IMM8,
                                             FVM_JIT_WIN64_SUB_RSP,
                                             FVM_JIT_WIN64_SHADOW_BYTES,
                                             FVM_JIT_SAVE_ARG_0,
                                             FVM_JIT_SAVE_ARG_1,
                                             FVM_JIT_WIN64_SAVE_RCX};
    static const unsigned char epilogue[] = {FVM_JIT_WIN64_REX_W,
                                             FVM_JIT_WIN64_GROUP_IMM8,
                                             FVM_JIT_WIN64_ADD_RSP,
                                             FVM_JIT_WIN64_SHADOW_BYTES,
                                             FVM_JIT_POP_R12_0,
                                             FVM_JIT_POP_R12_1,
                                             FVM_JIT_RET};
    static const unsigned char unwind[] = {FVM_JIT_WIN64_UNWIND_VERSION,
                                           FVM_JIT_WIN64_PROLOGUE_BYTES,
                                           FVM_JIT_WIN64_UNWIND_CODES,
                                           FVM_JIT_WIN64_UNWIND_FRAME,
                                           FVM_JIT_WIN64_ALLOC_END,
                                           FVM_JIT_WIN64_UNWIND_ALLOC,
                                           FVM_JIT_WIN64_PUSH_END,
                                           FVM_JIT_WIN64_UNWIND_PUSH};
    size_t aligned, table_offset, length;
    unsigned char *memory;
    PRUNTIME_FUNCTION table;
    DWORD old_protect;
    if (!bytes || !out || !mapped || size < sizeof(prologue) + sizeof(epilogue))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (size > FVM_JIT_MAX_CODE_BYTES)
    {
        return FVM_UNSUPPORTED;
    }
    if (memcmp(bytes, prologue, sizeof(prologue)) ||
        memcmp((const unsigned char *)bytes + size - sizeof(epilogue), epilogue, sizeof(epilogue)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    aligned = size +
              (FVM_JIT_WIN64_UNWIND_ALIGNMENT - size % FVM_JIT_WIN64_UNWIND_ALIGNMENT) % FVM_JIT_WIN64_UNWIND_ALIGNMENT;
    table_offset = aligned + sizeof(unwind);
    length = table_offset + sizeof(RUNTIME_FUNCTION);
    memory = (unsigned char *)VirtualAlloc(0, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memory)
    {
        return GetLastError() == ERROR_NOT_ENOUGH_MEMORY || GetLastError() == ERROR_OUTOFMEMORY ? FVM_OUT_OF_MEMORY
                                                                                                : FVM_UNSUPPORTED;
    }
    memcpy(memory, bytes, size);
    memset(memory + size, 0, length - size);
    memcpy(memory + aligned, unwind, sizeof(unwind));
    table = (PRUNTIME_FUNCTION)(memory + table_offset);
    table->BeginAddress = 0;
    table->EndAddress = (DWORD)size;
    table->UnwindData = (DWORD)aligned;
    if (!VirtualProtect(memory, length, PAGE_EXECUTE_READ, &old_protect) ||
        !FlushInstructionCache(GetCurrentProcess(), memory, size) ||
        !RtlAddFunctionTable(table, FVM_JIT_WIN64_FUNCTION_ENTRIES, (DWORD64)(ULONG_PTR)memory))
    {
        VirtualFree(memory, 0, MEM_RELEASE);
        return FVM_UNSUPPORTED;
    }
    *out = memory;
    *mapped = table_offset;
    return FVM_OK;
#else
    (void)bytes;
    (void)size;
    (void)out;
    (void)mapped;
    return FVM_UNSUPPORTED;
#endif
}


/* Unregister before releasing metadata/code; a rejected unregister retains the mapping safely. */
void fvm_port_code_release(void *memory, size_t mapped)
{
#if (defined(_M_X64) || defined(__x86_64__)) && !defined(_M_ARM64EC) && !defined(__arm64ec__)
    if (memory && mapped && RtlDeleteFunctionTable((PRUNTIME_FUNCTION)((unsigned char *)memory + mapped)))
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
#else
    (void)memory;
    (void)mapped;
#endif
}
