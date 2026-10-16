/* Failed resolver result snapshots never publish temporary name storage. */
#include "flashvm/standard.h"
#include "../src/core/string_internal.hpp"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
        throw std::bad_alloc();
    if (allowance > 0)
        --allowance;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete(void *p) throw()
{
    std::free(p);
}
void *operator new[](std::size_t size) throw(std::bad_alloc)
{
    return ::operator new(size);
}
void operator delete[](void *p) throw()
{
    ::operator delete(p);
}
void *operator new(std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return 0;
    }
}
void operator delete(void *p, const std::nothrow_t &) throw()
{
    ::operator delete(p);
}
void *operator new[](std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return 0;
    }
}
void operator delete[](void *p, const std::nothrow_t &) throw()
{
    ::operator delete[](p);
}
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include "flashvm/table.h"
#include "flashvm/exception.h"
static unsigned resolver_oom_throw, resolver_oom_calls;
static FVM_Result resolver_oom_result(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    ++resolver_oom_calls; assert(n == 1 && args[0].type == FVM_VALUE_STRING);
    if (resolver_oom_throw) { fvm_value_undefined(out); out->type = FVM_VALUE_OBJECT; out->object = (FVM_Object *)userdata; return fvm_exception_raise(c, out); }
    *out = args[0]; return FVM_OK;
}
static FVM_Result resolver_oom_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{ return resolver_oom_result(c, frame->receiver, frame->argc, frame->argv, out, frame->capture); }
static FVM_Value resolver_oom_value(FVM_Object *o)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = o; return value; }
static FVM_Result resolver_oom_read(FVM_Context *c, FVM_Object *receiver, unsigned indexed, FVM_Value *out)
{ return indexed ? fvm_table_get_index(c, receiver, 12, out) : fvm_object_get_bytes(c, receiver, "Request\0Key", 11, out); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned kind = 0; kind < 3; ++kind) for (unsigned indexed = 0; indexed < 2; ++indexed) for (resolver_oom_throw = 0; resolver_oom_throw < 2; ++resolver_oom_throw) {
        bool success = false, failed = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *receiver = 0, *capture = 0, *fn = 0; FVM_Value value, out;
            assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK && fvm_object_create(c, &capture) == FVM_OK);
            value = resolver_oom_value(capture); assert(fvm_object_set(c, capture, "self", &value) == FVM_OK);
            if (kind == 2) { assert(fvm_object_define_method_rooted(c, receiver, "__resolve", resolver_oom_result, capture, capture) == FVM_OK); }
            else {
                if (kind == 0) assert(fvm_function_create_native(c, resolver_oom_result, capture, capture, &fn) == FVM_OK);
                else { const unsigned char end[] = {0}; assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK && fvm_function_set_body(c, fn, resolver_oom_body, 0, capture) == FVM_OK); }
                value = resolver_oom_value(fn); assert(fvm_object_set(c, receiver, "__resolve", &value) == FVM_OK);
            }
            fvm_value_number(&out, 99); resolver_oom_calls = 0; allowance = boundary;
            FVM_Result status = resolver_oom_read(c, receiver, indexed, &out); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->resolve_depth && !c->calling_owner && !c->calling_function && !c->script_depth);
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(out.type == FVM_VALUE_UNDEFINED); }
            else { success = true; assert(status == (resolver_oom_throw ? FVM_EXCEPTION : FVM_OK) && resolver_oom_calls == 1); }
            if (c->exception_pending) { assert(resolver_oom_throw && fvm_exception_get(c, &value) == FVM_OK && value.object == capture && fvm_exception_clear(c) == FVM_OK); }
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            status = resolver_oom_read(c, receiver, indexed, &out); assert(status == (resolver_oom_throw ? FVM_EXCEPTION : FVM_OK));
            if (resolver_oom_throw) { assert(out.type == FVM_VALUE_UNDEFINED && fvm_exception_get(c, &value) == FVM_OK && value.object == capture && fvm_exception_clear(c) == FVM_OK); }
            else {
                assert(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == (indexed ? 2U : 11U) && !std::memcmp(out.string, indexed ? "12" : "Request\0Key", indexed ? 2 : 11));
                assert(fvm_set_global(c, "retainedReply", &out) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK && fvm_get_global(c, "retainedReply", &out) == FVM_OK && fvm_value_string_size(&out) == (indexed ? 2U : 11U));
            }
            assert(!c->gc_frames && !c->resolve_depth && !c->calling_owner); fvm_context_destroy(c);
        }
        assert(success && failed);
    }
    fvm_runtime_destroy(rt);
}
