#ifndef FLASHVM_SWF_H
#define FLASHVM_SWF_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FVM_SWFInfo { unsigned long struct_size; unsigned int version, frame_count, frame_rate_fixed8; unsigned long declared_length, tag_count, action_tag_count; } FVM_SWFInfo;
FVM_API FVM_Result fvm_swf_inspect(const void *, size_t, FVM_SWFInfo *);
FVM_API FVM_Result fvm_swf_info(FVM_Context *, FVM_SWFInfo *);
#ifdef __cplusplus
}
#endif
#endif
