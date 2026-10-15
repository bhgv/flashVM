/* Inject every allocation boundary in negative borrowed splice and verify cleanup. */
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
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK); bool succeeded = false;
    for (long failure = 0; failure < 4096 && !succeeded; ++failure) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *source = 0, *array = 0; FVM_Value value, args[5], out;
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK);
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_gc_pin(c, source) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        fvm_value_number(&value, -2); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args + 1, 1);
        fvm_value_string(args + 2, "first"); fvm_value_string(args + 3, "second"); fvm_value_number(args + 4, 33);
        FVM_Value method, call_args[6];
        assert(fvm_object_get(c, array, "splice", &method) == FVM_OK && method.type == FVM_VALUE_OBJECT);
        assert(fvm_gc_pin(c, method.object) == FVM_OK);
        call_args[0] = object(source);
        for (unsigned i = 0; i < 5; ++i) call_args[i + 1] = args[i];
        allowance = failure;
        FVM_Result status = fvm_object_call(c, method.object, "call", 6, call_args, &out);
        allowance = -1;
        assert(status == FVM_OK || status == FVM_OUT_OF_MEMORY);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
        succeeded = status == FVM_OK;
        if (succeeded) {
            number(c, source, "length", 1); number(c, source, "0", 33); number(c, out.object, "length", 0);
            assert(fvm_gc_pin(c, out.object) == FVM_OK);
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        if (succeeded) {
            assert(fvm_object_get(c, source, "-2", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "first"));
            assert(fvm_object_get(c, source, "-1", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "second"));
        } else {
            /* Retry after a nontransactional partial write with the original length. */
            fvm_value_number(&value, -2); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            assert(borrow(c, array, "splice", source, 5, args, &out) == FVM_OK);
            number(c, source, "length", 1); number(c, source, "0", 33);
        }
        fvm_context_destroy(c);
    }
    assert(succeeded); fvm_runtime_destroy(rt);
}
