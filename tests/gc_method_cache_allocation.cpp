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
#define main method_cache_fixture_main
#include "gc_method_cache.cpp"
#undef main
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const unsigned versions[] = {0, 6, 8};
    for (unsigned profile = 0; profile < 3; ++profile) for (unsigned operation = 0; operation < 4; ++operation) {
        bool success = false, failed = false;
        for (long boundary = 0; boundary < 256 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned version = versions[profile];
            if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
            assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *object = 0, *root = 0; FVM_Value value, old, out;
            assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK && fvm_object_create(c, &root) == FVM_OK);
            fvm_value_number(&value, 1); assert(fvm_object_set(c, root, "value", &value) == FVM_OK);
            assert(fvm_object_define_method_rooted_bytes(c, object, "MiXeD\0Name", 10, captured, root, root) == FVM_OK);
            assert(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &old) == FVM_OK && old.type == FVM_VALUE_OBJECT);
            const char *query = version == 6 ? "mixed\0name" : "MiXeD\0Name";
            FVM_Result status; int removed = 0; fvm_value_number(&value, 23);
            allowance = boundary;
            if (operation == 0) status = fvm_object_define_method_rooted_bytes(c, object, query, 10, replacement, 0, 0);
            else if (operation == 1) status = fvm_object_delete_bytes(c, object, query, 10, &removed);
            else if (operation == 2) status = fvm_object_set_bytes(c, object, query, 10, &value);
            else status = fvm_object_define_accessor_rooted_bytes(c, object, query, 10, replacement, 0, 0, 0);
            allowance = -1;
            if (status == FVM_OK) { success = true; if (operation == 1) assert(removed); }
            else {
                assert(status == FVM_OUT_OF_MEMORY); failed = true;
                assert(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &out) == FVM_OK && out.object == old.object);
                assert(fvm_function_invoke(c, old.object, object, 0, 0, &out) == FVM_OK && out.number == 1);
            }
            assert(fvm_gc_collect(c, 0) == FVM_OK && live_capture(c, root) == !success);
            assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
        }
        assert(success && failed);
    }
    fvm_runtime_destroy(rt);
}
