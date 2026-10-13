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
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <cassert>
#include <cstring>
static unsigned int reads;
static FVM_Result getter(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    ++reads;
    fvm_value_number(out, 4);
    return FVM_OK;
}
static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}
static FVM_Result borrow(FVM_Context *c, FVM_Object *array, const char *name, FVM_Object *receiver,
                         unsigned int n, const FVM_Value *args, FVM_Value *out)
{
    FVM_Value method, call[8];
    assert(n < 7);
    assert(fvm_object_get(c, array, name, &method) == FVM_OK && method.type == FVM_VALUE_OBJECT);
    call[0] = object(receiver);
    for (unsigned int i = 0; i < n; ++i)
        call[i + 1] = args[i];
    return fvm_object_call(c, method.object, "call", n + 1, call, out);
}
static void number(FVM_Context *c, FVM_Object *o, const char *name, double expected)
{
    FVM_Value v;
    assert(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER &&
           v.number == expected);
}
static unsigned names, writes;
static FVM_Result name_string(FVM_Context *, FVM_Object *, unsigned int,
                              const FVM_Value *, FVM_Value *out, void *)
{
    ++names;
    return fvm_value_string(out, "coerced");
}
static FVM_Result fail_name(FVM_Context *, FVM_Object *, unsigned int,
                            const FVM_Value *, FVM_Value *, void *) { return FVM_OUT_OF_MEMORY; }
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned int,
                         const FVM_Value *, FVM_Value *out, void *)
{ ++writes; fvm_value_undefined(out); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    FVM_Object *get = 0; FVM_Value args[3], value, out;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_function_create_native(c, getter, 0, 0, &get) == FVM_OK);
    assert(fvm_gc_pin(c, get) == FVM_OK);
    fvm_value_number(args, 2); args[1] = object(get);
    fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
    bool succeeded = false;
    for (long failure = 0; failure < 1024 && !succeeded; ++failure) {
        FVM_Object *o = 0;
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, 9);
        assert(fvm_object_set(c, o, "2", &value) == FVM_OK);
        allowance = failure;
        FVM_Result r = fvm_object_call(c, o, "addProperty", 3, args, &out);
        allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY);
        succeeded = r == FVM_OK;
        unsigned flags = 0;
        assert(fvm_object_property_flags(c, o, "2", &flags) == FVM_OK);
        if (succeeded) {
            assert(out.boolean && !(flags & FVM_PROPERTY_READ_ONLY));
            number(c, o, "2", 4);
        } else {
            assert(!(flags & FVM_PROPERTY_READ_ONLY));
            number(c, o, "2", 9);
        }
        assert(fvm_gc_unpin(c, o) == FVM_OK);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(succeeded);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
