/* FlashVM: Public embedding API declarations: exception.
 * File: include/flashvm/exception.h.
 */

#ifndef FLASHVM_EXCEPTION_H
/* Define FLASHVM EXCEPTION H constant or compilation switch. */
#define FLASHVM_EXCEPTION_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Stable machine codes; message text is English and also stable per code. */
/* Define FVM RESOURCE CLOSED constant or compilation switch. */
#define FVM_RESOURCE_CLOSED 1001UL
/* Define FVM RESOURCE STALE constant or compilation switch. */
#define FVM_RESOURCE_STALE 1002UL
/* Define FVM RESOURCE DISCONNECTED constant or compilation switch. */
#define FVM_RESOURCE_DISCONNECTED 1003UL
/* Define FVM RESOURCE UNREACHABLE constant or compilation switch. */
#define FVM_RESOURCE_UNREACHABLE 1004UL
/* Define FVM RESOURCE NOT OWNER constant or compilation switch. */
#define FVM_RESOURCE_NOT_OWNER 1005UL
/* Define FVM RESOURCE RECURSIVE constant or compilation switch. */
#define FVM_RESOURCE_RECURSIVE 1006UL
/* Define FVM RESOURCE CANCELLED constant or compilation switch. */
#define FVM_RESOURCE_CANCELLED 1007UL
/* Define FVM RESOURCE IO constant or compilation switch. */
#define FVM_RESOURCE_IO 1008UL
/* Define FVM RESOURCE FAILURE constant or compilation switch. */
#define FVM_RESOURCE_FAILURE 1009UL
/* Define FVM EXCEPTION THREAD constant or compilation switch. */
#define FVM_EXCEPTION_THREAD 1UL
/* Define FVM EXCEPTION FILE constant or compilation switch. */
#define FVM_EXCEPTION_FILE 2UL
/* Define FVM EXCEPTION NETWORK constant or compilation switch. */
#define FVM_EXCEPTION_NETWORK 4UL
#define FVM_EXCEPTION_REMOTE_CONTEXT                                                               \
    8UL /* Resource identifiers/path describe the remote authority. */
    /* Define FVM_ExceptionInfo struct and its representation. */
    typedef struct FVM_ExceptionInfo
    {
        unsigned long code /* Machine-readable exception or result code. */,
            resource /* Opaque shared or platform resource handle. */,
            thread /* Identifier of the associated worker. */,
            origin_thread /* Identifier of the resource originating thread. */,
            flags /* Bit flags controlling the operation. */,
            line /* Source line number associated with this entry. */,
            offset; /* Byte offset identifying the instruction or data. */
        char message[128] /* Human-readable diagnostic message. */,
            operation[32] /* Resource operation being performed or diagnosed. */,
            kind[16] /* Resource or event kind discriminator. */,
            path[512] /* Path associated with the resource. */,
            address[512]; /* Network address identifying the endpoint. */
        char source[256] /* Source name associated with this entry. */,
            function[256]; /* Function name associated with this source location. */
    } FVM_ExceptionInfo;
    /* Pending value is a GC root until clear. Returned values are borrowed. Any AS
       value may be thrown; info returns NOT_FOUND for ordinary script throws. */
    /* Read exception. */
    FVM_API FVM_Result fvm_exception_get(FVM_Context *, FVM_Value *);
    /* Implement exception info for this subsystem. */
    FVM_API FVM_Result fvm_exception_info(FVM_Context *, FVM_ExceptionInfo *);
    /* Clear exception. */
    FVM_API FVM_Result fvm_exception_clear(FVM_Context *);
    /* Implement exception raise for this subsystem. */
    FVM_API FVM_Result fvm_exception_raise(FVM_Context *, const FVM_Value *);
    /* Rich debugger intercept events; successful clear removes the copied batch. */
    /* Implement debug exception events for this subsystem. */
    FVM_API FVM_Result fvm_debug_exception_events(FVM_ExceptionInfo *, size_t, size_t *, int);
#ifdef __cplusplus
}
#endif
#endif
