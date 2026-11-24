/* Copyright (c) 2026 bhgv
 * EN: Entry adapters shared by named globals and retained object images.
 * RU: Общие адаптеры входов для глобальных функций и удерживаемых образов объекта.
 */
#ifndef FVM_STYXRPC_OBJECT_HANDLERS_H
#define FVM_STYXRPC_OBJECT_HANDLERS_H
#include "flashvm/styxrpc.h"
#include "flashvm/styxrpc_channel.h"
#include "flashvm/styxrpc_graph.h"

/* [ASINC] EN: Dispatch a scalar export to receiver or to the named global.
 * RU: Выполнить скалярный экспорт на receiver либо глобальную функцию. */
int fvm_styxrpc_run_int(const FVM_StyxRpcExport *binding, FVM_Object *receiver, int64_t argument, int64_t *result);

/* [ASINC] EN: Run the managed codec and dispatch on the optional receiver.
 * RU: Выполнить управляемый кодек и вызвать функцию на необязательном receiver. */
StyxRpcStatus fvm_styxrpc_run_managed(FVM_StyxRpcManagedExport *binding, FVM_Object *receiver,
    const FVM_SRecord *arguments, StyxRpcOutput *output);

/* [ASINC] EN: Run one positional graph export on the optional receiver.
 * RU: Выполнить один экспорт позиционного графа на необязательном receiver. */
StyxRpcStatus fvm_styxrpc_run_graph(FVM_StyxRpcManagedExport *binding, FVM_Object *receiver,
    const FVM_SRecord *arguments, StyxRpcGraphOutput *output);

/* [ASINC] EN: Run a script channel factory on the optional receiver.
 * RU: Выполнить фабрику канала на необязательном receiver. */
StyxRpcStatus fvm_styxrpc_run_channel(const FVM_StyxRpcChannelExport *binding, FVM_Object *receiver,
    const StyxRpcChannelCall *call, StyxRpcChannelOutput *output);
#endif
