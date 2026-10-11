#include "../core/runtime.hpp"
#include <new>
#include <cstring>
#include "flashvm/metamethod.h"
extern "C" {
unsigned long fvm_abi_version(void) { return FVM_ABI_VERSION; }
FVM_Result fvm_config_init(FVM_Config *config) {
    if (!config) return FVM_INVALID_ARGUMENT;
    std::memset(config, 0, sizeof(*config));
    config->struct_size = (unsigned long)sizeof(*config);
    return FVM_OK;
}
FVM_Result fvm_runtime_create(const FVM_Config *config, FVM_Runtime **out_runtime) {
    FVM_Runtime *runtime;
    if (!out_runtime) return FVM_INVALID_ARGUMENT;
    *out_runtime = 0;
    if (!config || config->struct_size != sizeof(FVM_Config)) return FVM_INVALID_ARGUMENT;
    if (config->features != 0) return FVM_UNSUPPORTED;
    runtime = new (std::nothrow) FVM_Runtime;
    if (!runtime) return FVM_OUT_OF_MEMORY;
    runtime->config = *config;
    *out_runtime = runtime;
    return FVM_OK;
}
void fvm_runtime_destroy(FVM_Runtime *runtime) { delete runtime; }
FVM_Result fvm_context_create(FVM_Runtime *runtime, FVM_Context **out_context) {
    FVM_Context *context;
    if (!out_context) return FVM_INVALID_ARGUMENT;
    *out_context = 0;
    if (!runtime) return FVM_INVALID_ARGUMENT;
    context = new (std::nothrow) FVM_Context;
    if (!context) return FVM_OUT_OF_MEMORY;
    context->worker=0;
    context->runtime = runtime;
    context->global = 0;
    context->swf_loaded = false;
    context->script_depth = 0;
    context->gc_mode = 0;
    context->gc_allocations = 0;
    context->gc_collecting = false;
    context->destroying = false;
    context->gc_releasing = false;
    context->gc_frames = 0;
    context->gc_native_depth = 0;
    context->pumping_input = false;
    context->input_callback = 0;
    context->input_userdata = 0;
    if (fvm_object_create(context, &context->global) != FVM_OK) { delete context; return FVM_OUT_OF_MEMORY; }
    *out_context = context;
    return FVM_OK;
}
void fvm_context_destroy(FVM_Context *context) {
    if (!context || context->destroying || context->gc_collecting || context->gc_native_depth) return;
    context->destroying=true;
    context->gc_collecting=true;
    for (size_t i=0;i<context->objects.size();++i) fvm_meta_finalize(context,context->objects[i]);
    for (size_t i=0;i<context->objects.size();++i)
        fvm_release_object_resource(context,context->objects[i]);
    for (size_t i=0;i<context->objects.size();++i) delete context->objects[i];
    for (size_t i=0;i<context->strings.size();++i) delete [] context->strings[i];
    delete context;
}
FVM_Result fvm_load_memory(FVM_Context *context, const void *data, size_t length) {
    if (!context || (!data && length)) return FVM_INVALID_ARGUMENT;
    FVM_SWFInfo info;
    info.struct_size=sizeof(info);
    if (fvm_swf_inspect(data,length,&info)!=FVM_OK) return fvm_swf_inspect(data,length,&info);
    try { context->swf_bytes.assign((const unsigned char *)data,(const unsigned char *)data+length); }
    catch (...) { return FVM_OUT_OF_MEMORY; }
    context->swf_info=info;
    context->swf_loaded=true;
    return FVM_OK;
}
FVM_Result fvm_advance(FVM_Context *context, double milliseconds) {
    if (!context || milliseconds < 0.0) return FVM_INVALID_ARGUMENT;
    return FVM_UNSUPPORTED;
}
}
