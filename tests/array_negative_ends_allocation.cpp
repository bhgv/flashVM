/* Inject every allocation boundary in signed borrowed end operations and verify cleanup. */
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
    const char *methods[] = {"push", "unshift", "pop", "shift"};
    for (unsigned method_index = 0; method_index < 4; ++method_index) {
        bool succeeded = false;
        for (long failure = 0; failure < 4096 && !succeeded; ++failure) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *source = 0, *array = 0; FVM_Value value, args[3], out, method;
            assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK);
            assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
            assert(fvm_gc_pin(c, source) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
            fvm_value_number(&value, -2); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            fvm_value_number(&value, 81); assert(fvm_object_set(c, source, "-3", &value) == FVM_OK);
            fvm_value_number(&value, 82); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
            assert(fvm_object_get(c, array, methods[method_index], &method) == FVM_OK && method.type == FVM_VALUE_OBJECT);
            assert(fvm_gc_pin(c, method.object) == FVM_OK);
            args[0] = object(source); fvm_value_string(args + 1, "first"); fvm_value_string(args + 2, "second");
            unsigned argc = method_index < 2 ? 3 : 1;
            allowance = failure;
            FVM_Result status = fvm_object_call(c, method.object, "call", argc, args, &out);
            allowance = -1;
            assert(status == FVM_OK || status == FVM_OUT_OF_MEMORY);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            succeeded = status == FVM_OK;
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            if (succeeded) {
                assert(out.type == FVM_VALUE_NUMBER && out.number == (method_index < 2 ? 0 : method_index == 2 ? 81 : 82));
                number(c, source, "length", method_index == 0 ? 0 : -2);
                if (method_index < 2) {
                    assert(fvm_object_get(c, source, method_index == 0 ? "-2" : "0", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "first"));
                    assert(fvm_object_get(c, source, method_index == 0 ? "-1" : "1", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "second"));
                }
            } else {
                fvm_value_number(&value, -2); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
                assert(fvm_object_call(c, method.object, "call", argc, args, &out) == FVM_OK);
            }
            fvm_context_destroy(c);
        }
        assert(succeeded);
    }
    fvm_runtime_destroy(rt);
}
