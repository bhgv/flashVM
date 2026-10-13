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
#include "flashvm/function.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    FVM_Object *array = 0; FVM_Value value, out;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(&value, 17.0);
    assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK);
    for (unsigned long i = 0; i < 8; ++i) {
        fvm_value_number(&value, (double)i);
        assert(fvm_table_set_index(c, array, i * 2, &value) == FVM_OK);
    }
    bool succeeded = false;
    for (long failure = 0; failure < 4096 && !succeeded; ++failure) {
        allowance = failure;
        FVM_Result r = fvm_object_call(c, array, "join", 0, 0, &out);
        allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY);
        assert(c->avm1_execution_depth == 0 && c->joining_arrays.empty());
        assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.number == 17.0);
        assert(fvm_table_get_index(c, array, 7 * 2, &value) == FVM_OK && value.number == 7);
        succeeded = r == FVM_OK;
        if (succeeded)
            assert(out.type == FVM_VALUE_STRING);
    }
    assert(succeeded);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
