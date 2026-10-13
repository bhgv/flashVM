/* FlashVM: Public embedding API declarations: avm1.
 * File: include/flashvm/avm1.h.
 */

#ifndef FLASHVM_AVM1_H
/* Define FLASHVM AVM1 H constant or compilation switch. */
#define FLASHVM_AVM1_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Define FVM_AVM1Info struct and its representation. */
    typedef struct FVM_AVM1Info
    {
        unsigned long struct_size;       /* Size of this structure for API compatibility checks. */
        unsigned long instruction_count; /* Number of decoded bytecode instructions. */
        unsigned long push_count;        /* Number of decoded pushed values. */
        unsigned long branch_count;      /* Number of decoded branch instructions. */
    } FVM_AVM1Info;
    /* Validates instruction framing, not the full AVM1 operand semantics. */
    /* Validate AVM1 instruction framing and count instructions, pushes and branches. */
    FVM_API FVM_Result fvm_avm1_inspect(const void *data, size_t size, FVM_AVM1Info *info);
    /* Executes the supported AVM1 subset; unsupported opcodes return FVM_UNSUPPORTED. */
    /* Execute avm1. */
    FVM_API FVM_Result fvm_avm1_execute(FVM_Context *, const void *, size_t);
    /* [GC:] Set the ActionTrace sink and optional same-context retained capture.
     * Callback receives one STRING argument and the global receiver; its result is ignored.
     * NULL callback with NULL userdata/capture disables delivery. Do not destroy the context
     * from the callback. Text is borrowed for the synchronous call; retain a copy if needed. */
    FVM_API FVM_Result fvm_avm1_set_trace_rooted(FVM_Context *, FVM_NativeFunction, void *,
                                                 FVM_Object *);
    /* Execute avm1 receiver. */
    FVM_API FVM_Result fvm_avm1_execute_receiver(FVM_Context *, const void *, size_t, FVM_Object *);
    /* Invoke a named AVM1 function with rooted parameter bindings and receiver. */
    /* Call avm1. */
    FVM_API FVM_Result fvm_avm1_call(FVM_Context *, const char *, FVM_Object *, unsigned int,
                                     const FVM_Value *, FVM_Value *);
    /* Invoke a complete counted AVM1/global Function key with the same receiver contract. */
    FVM_API FVM_Result fvm_avm1_call_bytes(FVM_Context *, const char *, size_t, FVM_Object *,
                                           unsigned int, const FVM_Value *, FVM_Value *);
    /* Executes top-level DoAction tags from the loaded FWS file, in file order. */
    /* Execute swf actions. */
    FVM_API FVM_Result fvm_swf_execute_actions(FVM_Context *);
#ifdef __cplusplus
}
#endif
#endif
