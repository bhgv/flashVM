#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
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

int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c, *other;
    FVM_Object *object;
    FVM_Value value, out, graph;
    const char key[] = {'x', 0, (char)0xed, (char)0xa0, (char)0x80,
                        (char)0xed, (char)0xb0, (char)0x80};
    bool success = false;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_object_create(c, &object) == FVM_OK);
    assert(fvm_gc_pin(c, object) == FVM_OK);
    for (long failure = 0; failure < 256 && !success; ++failure)
    {
        fvm_value_number(&value, 7);
        assert(fvm_object_set_bytes(c, object, key, sizeof(key), &value) == FVM_OK);
        fvm_value_number(&value, 42);
        allowance = failure;
        FVM_Result result = fvm_object_set_bytes(c, object, key, sizeof(key), &value);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        success = result == FVM_OK;
        assert(fvm_object_get_bytes(c, object, key, sizeof(key), &out) == FVM_OK);
        assert(out.number == (success ? 42 : 7));
    }
    assert(success);
    success = false;
    for (long failure = 0; failure < 256 && !success; ++failure)
    {
        fvm_value_number(&out, 99);
        allowance = failure;
        FVM_Result result = fvm_object_get_bytes(c, object, key, sizeof(key), &out);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        success = result == FVM_OK;
        assert(success ? out.number == 42 : out.type == FVM_VALUE_UNDEFINED);
    }
    assert(success);
    fvm_value_undefined(&graph);
    graph.type = FVM_VALUE_OBJECT;
    graph.object = object;
    unsigned char *bytes = 0;
    size_t size = 0;
    assert(fvm_value_serialize(c, &graph, &bytes, &size) == FVM_OK);
    success = false;
    for (long failure = 0; failure < 2048 && !success; ++failure)
    {
        unsigned long before, after;
        assert(fvm_gc_object_count(other, &before) == FVM_OK);
        allowance = failure;
        FVM_Result result = fvm_value_deserialize(other, bytes, size, &out);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        success = result == FVM_OK;
        assert(fvm_gc_object_count(other, &after) == FVM_OK);
        if (!success)
            assert(before == after && out.type == FVM_VALUE_UNDEFINED);
        else
        {
            FVM_Value restored;
            assert(fvm_object_get_bytes(other, out.object, key, sizeof(key), &restored) == FVM_OK);
            assert(restored.number == 42);
        }
    }
    assert(success);
    fvm_serialized_free(bytes);
    assert(fvm_gc_unpin(c, object) == FVM_OK);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
