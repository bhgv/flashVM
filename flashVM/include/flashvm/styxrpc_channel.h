/* Copyright (c) 2026 bhgv
 * EN: Import scoped Dis functions returning live native UniBus capabilities.
 * RU: Импорт контекстных функций Dis, возвращающих живые полномочия UniBus.
 */
#ifndef FLASHVM_STYXRPC_CHANNEL_H
#define FLASHVM_STYXRPC_CHANNEL_H
#include "flashvm/value.h"
#include "styxrpc/channel_client.h"
#include "styxrpc/channel_module.h"

#ifdef __cplusplus
extern "C" {
#endif


/* [GC:] EN: Register a synchronous global on its owner; borrow target while callable.
 * RU: Зарегистрировать синхронную глобальную функцию; заимствовать target, пока она доступна. */
FVM_API FVM_Result fvm_styxrpc_channel_import(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);

#define FVM_STYXRPC_CHANNEL_QUEUE_SLOTS 8U /* Maximum FIFO slots for one FlashVM export. */

typedef struct FVM_StyxRpcChannelExport
{
    FVM_Context *context;    /* Owner running the server and the named AS initializer. */
    const char *initializer; /* Callable AS function returning a seed or undefined. */
    StyxRpcSchema schema;     /* Complete compiled value-input, single-channel result. */
    unsigned capacity;       /* FIFO slots, zero for synchronous rendezvous. */
} FVM_StyxRpcChannelExport;


/* [GC:] EN: Create a scoped channel descriptor borrowing binding and names through teardown.
 * RU: Создать дескриптор контекстного канала, заимствуя привязку и имена до teardown. */
FVM_API FVM_Result fvm_styxrpc_channel_export(FVM_StyxRpcChannelExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);

#ifdef __cplusplus
}
#endif
#endif
