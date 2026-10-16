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
#define main property_case_definition_fixture_main
#include "object_property_case_definition.cpp"
#undef main
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *getter = 0; const unsigned char end[] = {0}; FVM_Value value, args[3], out;
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK && fvm_gc_pin(c, getter) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, object, "MiXeD\0Name", 10, &value) == FVM_OK);
        assert(fvm_object_set_property_flags_bytes(c, object, "MiXeD\0Name", 10, 3, 0) == FVM_OK);
        fvm_value_string_bytes(args, "mixed\0name", 10); args[1] = definition_object(getter); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
        bool success = false, failed = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            allowance = boundary; FVM_Result status = fvm_object_call(c, object, "addProperty", 3, args, &out); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth);
            if (status == FVM_OK) { assert(out.boolean); success = true; }
            else {
                assert(status == FVM_OUT_OF_MEMORY); failed = true;
                assert(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &out) == FVM_OK && out.number == 9);
                unsigned flags = 0; assert(fvm_object_property_flags_bytes(c, object, "MiXeD\0Name", 10, &flags) == FVM_OK && flags == 3);
            }
            assert(fvm_gc_collect(c, 0) == FVM_OK);
        }
        assert(success && failed); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
