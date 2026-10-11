#ifndef FLASHVM_AVM1_H
#define FLASHVM_AVM1_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_AVM1Info
    {
        unsigned long struct_size;
        unsigned long instruction_count;
        unsigned long push_count;
        unsigned long branch_count;
    } FVM_AVM1Info;
    /* Validates instruction framing, not the full AVM1 operand semantics. */
    FVM_API FVM_Result fvm_avm1_inspect(const void *data, size_t size, FVM_AVM1Info *info);
    /* Executes the supported AVM1 subset; unsupported opcodes return FVM_UNSUPPORTED. */
    FVM_API FVM_Result fvm_avm1_execute(FVM_Context *, const void *, size_t);
    FVM_API FVM_Result fvm_avm1_execute_receiver(FVM_Context *, const void *, size_t, FVM_Object *);
    /* Invoke a named AVM1 function with rooted parameter bindings and receiver. */
    FVM_API FVM_Result fvm_avm1_call(FVM_Context *, const char *, FVM_Object *, unsigned int,
                                     const FVM_Value *, FVM_Value *);
    /* Executes top-level DoAction tags from the loaded FWS file, in file order. */
    FVM_API FVM_Result fvm_swf_execute_actions(FVM_Context *);
#ifdef __cplusplus
}
#endif
#endif
