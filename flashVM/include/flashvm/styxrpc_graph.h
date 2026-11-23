/* Copyright (c) 2026 bhgv
 * EN: Bind native Dis positional ref graphs to ActionScript owner contexts.
 * RU: Привязка позиционных графов ref Dis к контекстам владельцев ActionScript.
 */
#ifndef FLASHVM_STYXRPC_GRAPH_H
#define FLASHVM_STYXRPC_GRAPH_H
#include "flashvm/styxrpc_managed.h"
#include "styxrpc/graph_module.h"

#ifdef __cplusplus
extern "C" {
#endif

/* EN: Register a synchronous AS graph import. Borrow target while callable.
 * RU: Зарегистрировать синхронный импорт графа AS и заимствовать target до конца вызовов. */
FVM_API FVM_Result fvm_styxrpc_import_graph(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);


/* EN: Create a synchronous graph descriptor; borrow binding and names through teardown.
 * RU: Создать синхронный описатель графа; заимствовать binding и имена до остановки. */
FVM_API FVM_Result fvm_styxrpc_export_graph(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcGraphExport *exported);

#ifdef __cplusplus
}
#endif
#endif
