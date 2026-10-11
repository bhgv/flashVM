#include "flashvm/meta_call.h"
#include "flashvm/table.h"
#include "flashvm/callable.h"
#include "../core/runtime.hpp"
#include <limits.h>

/* This bridge deliberately does not alter AVM1 bytecode semantics yet. */
extern "C" FVM_Result fvm_meta_call(FVM_Context *context, FVM_Object *owner,
    unsigned int count, const FVM_Value *arguments, FVM_Value *result)
{
    FVM_Object *meta = 0;
    FVM_Object *list = 0;
    FVM_Value params[3];
    FVM_Value entry;
    FVM_Result status;
    unsigned int i;
    if (!context || !owner || !result || (count && !arguments) ||
        owner->owner != context) return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(result);
    status = fvm_table_get_metatable(context, owner, &meta);
    if (status != FVM_OK) return status;
    if (!meta) return FVM_UNSUPPORTED;
    status = fvm_object_create(context, &list);
    if (status != FVM_OK) return status;
    for (i = 0; i < count; ++i) {
        status = fvm_table_set_index(context, list, (unsigned long)i, &arguments[i]);
        if (status != FVM_OK) return status;
    }
    fvm_value_undefined(&params[0]);
    params[0].type = FVM_VALUE_OBJECT; params[0].object = owner;
    params[1] = params[0]; params[1].object = meta;
    params[2] = params[0]; params[2].object = list;
    status = fvm_object_get(context, meta, "__call", &entry);
    if (status != FVM_OK) return status;
    if (entry.type == FVM_VALUE_OBJECT && entry.object)
        return fvm_callable_invoke(context, entry.object, 3, params, result);
    /* Legacy native callbacks registered on the metatable are supported too. */
    fvm_detail::map<fvm_detail::string,FVM_NativeEntry>::iterator it =
        meta->metamethods.find("__call");
    if (it == meta->metamethods.end()) return FVM_UNSUPPORTED;
    try { return it->second.callback(context, owner, 3, params, result, it->second.userdata); }
    catch (...) { return FVM_ERROR; }
}
