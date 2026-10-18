/* Inject every allocation boundary in budgeted large loaded sort snapshots and verify cleanup. */
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
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
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned variant = 0; variant < 4; ++variant) {
        bool reached_budget = false;
        for (long failure = 0; failure < 4096 && !reached_budget; ++failure) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *source = 0, *array = 0; FVM_Value value, args[3], out, method;
            assert(fvm_construct(c, variant & 1 ? "Object" : "Array", 0, 0, &source) == FVM_OK);
            assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
            assert(fvm_gc_pin(c, source) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
            fvm_value_number(&value, 65537); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            fvm_value_string(&value, "kept"); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
            assert(fvm_object_get(c, array, variant < 2 ? "sort" : "sortOn", &method) == FVM_OK && method.type == FVM_VALUE_OBJECT);
            assert(fvm_gc_pin(c, method.object) == FVM_OK);
            args[0] = object(source);
            if (variant < 2) fvm_value_number(args + 1, 16);
            else { fvm_value_string(args + 1, "field"); fvm_value_number(args + 2, 16); }
            unsigned argc = variant < 2 ? 2 : 3;
            unsigned long old_limit = rt->config.instruction_limit; rt->config.instruction_limit = 8;
            allowance = failure;
            FVM_Result status = fvm_object_call(c, method.object, "call", argc, args, &out);
            allowance = -1; rt->config.instruction_limit = old_limit;
            assert(status == FVM_ERROR || status == FVM_OUT_OF_MEMORY);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            reached_budget = status == FVM_ERROR;
            assert(fvm_gc_collect(c, 0) == FVM_OK); number(c, source, "length", 65537);
            assert(fvm_object_get(c, source, "0", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "kept"));
            rt->config.instruction_limit = 8;
            assert(fvm_object_call(c, method.object, "call", argc, args, &out) == FVM_ERROR);
            rt->config.instruction_limit = old_limit;
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
        }
        assert(reached_budget);
    }
    fvm_runtime_destroy(rt);
}
