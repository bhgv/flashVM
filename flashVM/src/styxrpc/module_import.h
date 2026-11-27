/* Copyright (c) 2026 bhgv
 * EN: Common call adapters for globals and loaded module methods.
 * RU: Общие адаптеры вызовов глобальных функций и методов загруженного модуля.
 */
#ifndef FVM_STYXRPC_MODULE_IMPORT_H
#define FVM_STYXRPC_MODULE_IMPORT_H
#include "flashvm/styxrpc_module.h"
#include "flashvm/styxrpc.h"
#include "flashvm/styxrpc_channel.h"

typedef struct FVM_StyxModuleLease FVM_StyxModuleLease; /* Context-owned request fid reference. */

/* EN: Retain or release a context-local module lease. NULL is accepted.
 * RU: Удержать либо освободить локальную аренду модуля. NULL допустим. */
void fvm_styxrpc_live_lease_retain(FVM_StyxModuleLease *lease);
void fvm_styxrpc_live_lease_release(FVM_StyxModuleLease *lease);


/* EN: Invoke a scalar target from state. / RU: Вызвать скалярную цель из state. */
FVM_Result fvm_styxrpc_invoke_int(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *);

/* EN: Invoke a managed target from state. / RU: Вызвать управляемую цель из state. */
FVM_Result fvm_styxrpc_invoke_managed(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *);

/* EN: Invoke a graph target from state. / RU: Вызвать цель графа из state. */
FVM_Result fvm_styxrpc_invoke_graph(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *);

/* EN: Invoke a channel binding from state, optionally lending arguments.
 * RU: Вызвать привязку каналов из state, при необходимости передать аргументы. */
FVM_Result fvm_styxrpc_invoke_channels(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *);

/* EN/RU: Borrow an exact module fid, or use the ordinary path when NULL.
 * Заимствовать точный fid модуля. При NULL использовать обычный путь. */
FVM_Result fvm_styxrpc_int_at(FVM_Context *, struct StyxFid *, const FVM_StyxRpcImport *,
    unsigned, const FVM_Value *, FVM_Value *);
FVM_Result fvm_styxrpc_managed_at(FVM_Context *, struct StyxFid *, const StyxRpcManagedTarget *,
    unsigned, const FVM_Value *, FVM_Value *);
FVM_Result fvm_styxrpc_graph_at(FVM_Context *, struct StyxFid *, const StyxRpcManagedTarget *,
    unsigned, const FVM_Value *, FVM_Value *);
FVM_Result fvm_styxrpc_channels_at(FVM_Context *, struct StyxFid *, const FVM_StyxRpcChannelArguments *,
unsigned, const FVM_Value *, FVM_Value *, FVM_StyxModuleLease *);

 /* [GC:] EN: Validate and load on a clone of the held unopened module directory.
 * Consume the optional request fid only on success and clear its caller slot.
 * RU: Проверить и загрузить на клоне удерживаемого неоткрытого каталога.
 * Принять необязательный fid запроса только при успехе и очистить слот вызывающего. */
FVM_Result fvm_styxrpc_module_load_at(FVM_Context *, struct StyxFid *, const FVM_StyxRpcLoadSpec *,
    struct StyxFid **lease, FVM_Value *);
#endif
