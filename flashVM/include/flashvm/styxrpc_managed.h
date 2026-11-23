/* Copyright (c) 2026 bhgv
 * EN: Bind value-only managed Styx RPC to ActionScript.
 * RU: Связь управляемого Styx RPC по значению с ActionScript.
 */
#ifndef FLASHVM_STYXRPC_MANAGED_H
#define FLASHVM_STYXRPC_MANAGED_H
#include "flashvm.h"
#include "styxrpc/managed_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FVM_StyxRpcManagedExport
{
    FVM_Context *context;          /* Serving owner context. */
    const char *function;          /* Named ActionScript implementation. */
    StyxRpcSchema schema;          /* Exact compiled Dis arguments/result. */
    unsigned char *result_buffer;  /* Host-owned encoded output staging. */
    size_t result_capacity;        /* Result buffer size in bytes. */
} FVM_StyxRpcManagedExport;


/* EN: Register a synchronous AS import, borrowing target while callable.
 * RU: Зарегистрировать синхронный импорт AS, заимствуя target до конца использования. */
FVM_API FVM_Result fvm_styxrpc_import_managed(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);


/* EN: Prepare a descriptor borrowing binding, names and result staging through teardown.
 * RU: Подготовить описатель, заимствующий binding, имена и буфер до остановки сервера. */
FVM_API FVM_Result fvm_styxrpc_export_managed(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcManagedExport *exported);

#ifdef __cplusplus
}
#endif
#endif
