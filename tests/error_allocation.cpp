#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/mutex.h"
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
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    bool installed = false, constructed = false;
    for (long failure = 0; failure < 512 && !installed; ++failure)
    {
        FVM_Context *c = 0;
        FVM_Value value;
        assert(fvm_context_create(runtime, &c) == FVM_OK);
        allowance = failure;
        FVM_Result result = fvm_standard_install(c);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        if (result == FVM_OK)
            installed = true;
        else
        {
            const char *names[] = {"Math", "Boolean", "Number", "Error", "NaN", "Infinity"};
            for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
                assert(fvm_get_global(c, names[i], &value) == FVM_OK &&
                       value.type == FVM_VALUE_UNDEFINED);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_standard_install(c) == FVM_OK);
        }
        fvm_context_destroy(c);
    }
    assert(installed);
    for (long failure = 0; failure < 64 && !constructed; ++failure)
    {
        FVM_Context *c = 0;
        FVM_Object *error = 0;
        FVM_Value message, text;
        assert(fvm_context_create(runtime, &c) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK);
        fvm_value_string(&message, "allocation checkpoint");
        allowance = failure;
        FVM_Result result = fvm_construct(c, "Error", 1, &message, &error);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        if (result == FVM_OK)
        {
            constructed = true;
            assert(error);
            assert(fvm_object_call(c, error, "toString", 0, 0, &text) == FVM_OK);
        }
        else
        {
            assert(!error);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_construct(c, "Error", 1, &message, &error) == FVM_OK);
        }
        fvm_context_destroy(c);
    }
    assert(constructed);
    bool materialized = false;
    for (long failure = 0; failure < 256 && !materialized; ++failure)
    {
        FVM_Context *c = 0;
        FVM_Mutex *mutex = 0;
        FVM_Value error, code;
        assert(fvm_context_create(runtime, &c) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_mutex_create(c, &mutex) == FVM_OK);
        assert(fvm_mutex_close(mutex) == FVM_OK);
        assert(fvm_mutex_set(c, mutex, 0) == FVM_ERROR);
        allowance = failure;
        FVM_Result result = fvm_exception_get(c, &error);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        if (result == FVM_OK)
            materialized = true;
        else
        {
            assert(error.type == FVM_VALUE_UNDEFINED);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_exception_get(c, &error) == FVM_OK);
        }
        assert(fvm_object_get(c, error.object, "code", &code) == FVM_OK && code.number == 1001);
        assert(fvm_exception_clear(c) == FVM_OK);
        fvm_mutex_release(mutex);
        fvm_context_destroy(c);
    }
    assert(materialized);
    fvm_runtime_destroy(runtime);
}
