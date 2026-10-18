/* Count queries validate complete WTF-8 without allocating expanded UTF-16. */
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
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/base_state.hpp"
static FVM_Result allocation_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = frame->capture;
    return fvm_exception_raise(c, &value);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *thrown = 0, *fn = 0; FVM_Value args[2], value, out; const unsigned char end[] = {0};
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK && fvm_object_create(c, &thrown) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK && fvm_function_set_body(c, fn, allocation_watch, 0, thrown) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, object, "slot", &value) == FVM_OK);
        fvm_value_string(args, "slot"); fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_OBJECT; args[1].object = fn;
        assert(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean);
        bool success = false, failed = false, pending_oom = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            fvm_value_number(&value, 9); assert(fvm_base_store_data(c, object, "slot", value, false) == FVM_OK);
            fvm_value_number(&value, 81); allowance = boundary;
            FVM_Result status = fvm_object_set(c, object, "slot", &value); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty());
            if (status == FVM_EXCEPTION) {
                success = true; assert(fvm_exception_get(c, &out) == FVM_OK && out.object == thrown);
                assert(fvm_object_get(c, object, "slot", &out) == FVM_OK);
                if (version) assert(out.type == FVM_VALUE_UNDEFINED); else assert(out.type == FVM_VALUE_NUMBER && out.number == 9);
            } else {
                assert(status == FVM_OUT_OF_MEMORY); failed = true;
                assert(fvm_object_get(c, object, "slot", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 9);
                if (c->exception_pending) { pending_oom = true; assert(fvm_exception_get(c, &out) == FVM_OK && out.object == thrown); }
            }
            assert(fvm_exception_clear(c) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        }
        assert(success && failed && (!version || pending_oom)); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
