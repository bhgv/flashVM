#ifndef FLASHVM_EXCEPTION_H
#define FLASHVM_EXCEPTION_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Stable machine codes; message text is English and also stable per code. */
#define FVM_RESOURCE_CLOSED 1001UL
#define FVM_RESOURCE_STALE 1002UL
#define FVM_RESOURCE_DISCONNECTED 1003UL
#define FVM_RESOURCE_UNREACHABLE 1004UL
#define FVM_RESOURCE_NOT_OWNER 1005UL
#define FVM_RESOURCE_RECURSIVE 1006UL
#define FVM_RESOURCE_CANCELLED 1007UL
#define FVM_RESOURCE_IO 1008UL
#define FVM_RESOURCE_FAILURE 1009UL
#define FVM_EXCEPTION_THREAD 1UL
#define FVM_EXCEPTION_FILE 2UL
#define FVM_EXCEPTION_NETWORK 4UL
    typedef struct FVM_ExceptionInfo
    {
        unsigned long code, resource, thread, origin_thread, flags, line, offset;
        char message[128], operation[32], kind[16], path[512], address[512];
        char source[256], function[256];
    } FVM_ExceptionInfo;
    /* Pending value is a GC root until clear. Returned values are borrowed. Any AS
       value may be thrown; info returns NOT_FOUND for ordinary script throws. */
    FVM_API FVM_Result fvm_exception_get(FVM_Context *, FVM_Value *);
    FVM_API FVM_Result fvm_exception_info(FVM_Context *, FVM_ExceptionInfo *);
    FVM_API FVM_Result fvm_exception_clear(FVM_Context *);
    FVM_API FVM_Result fvm_exception_raise(FVM_Context *, const FVM_Value *);
    /* Rich debugger intercept events; successful clear removes the copied batch. */
    FVM_API FVM_Result fvm_debug_exception_events(FVM_ExceptionInfo *, size_t, size_t *, int);
#ifdef __cplusplus
}
#endif
#endif
