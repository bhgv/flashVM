/* Copyright (c) 2026 bhgv
 * EN: Common call adapters for globals and loaded module methods.
 * RU: Общие адаптеры вызовов глобальных функций и методов загруженного модуля.
 */
#ifndef FVM_STYXRPC_MODULE_IMPORT_H
#define FVM_STYXRPC_MODULE_IMPORT_H
#include "flashvm/styxrpc_module.h"

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
#endif
