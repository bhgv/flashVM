#ifndef FLASHVM_AVM1_H
#define FLASHVM_AVM1_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FVM_AVM1Info {
    unsigned long struct_size;
    unsigned long instruction_count;
    unsigned long push_count;
    unsigned long branch_count;
} FVM_AVM1Info;
/* Validates instruction framing, not the full AVM1 operand semantics. */
FVM_API FVM_Result fvm_avm1_inspect(const void *data, size_t size, FVM_AVM1Info *info);
#ifdef __cplusplus
}
#endif
#endif
