/* Exhaustive loaded sort allocation failures restore roots and release temporary storage. */
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1, active_allocations;
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
    ++active_allocations;
    return p;
}
void operator delete(void *p) throw()
{
    if (p) --active_allocations;
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
/* Read actual stored ranks without relying on host roots for values displaced by partial copyback. */
static int rank(FVM_Context *c, FVM_Object *array, unsigned index, bool fields)
{
    FVM_Value value;
    assert(fvm_table_get_index(c, array, index, &value) == FVM_OK);
    if (fields) { assert(value.type == FVM_VALUE_OBJECT); assert(fvm_object_get(c, value.object, "rank", &value) == FVM_OK); }
    assert(value.type == FVM_VALUE_NUMBER); return (int)value.number;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const long baseline = active_allocations;
    const int initial[] = {3,1,4,2}, indices[] = {1,3,0,2};
    for (unsigned mode = 0; mode < 4; ++mode) {
        bool succeeded = false; unsigned failures = 0;
        for (long failure = 0; failure < 1024 && !succeeded; ++failure) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Value input[4], value, args[2], out; FVM_Object *array = 0;
            for (unsigned i = 0; i < 4; ++i) {
                fvm_value_number(input + i, initial[i]);
                if (mode == 3) {
                    FVM_Object *row = 0; assert(fvm_construct(c, "Object", 0, 0, &row) == FVM_OK);
                    assert(fvm_object_set(c, row, "rank", input + i) == FVM_OK);
                    fvm_value_undefined(input + i); input[i].type = FVM_VALUE_OBJECT; input[i].object = row;
                }
            }
            assert(fvm_construct(c, "Array", 4, input, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
            const char *method = mode == 3 ? "sortOn" : "sort";
            assert(fvm_object_get(c, array, method, &value) == FVM_OK);
            if (mode == 3) { fvm_value_string(args, "rank"); fvm_value_number(args + 1, 0); }
            else fvm_value_number(args, mode == 1 ? 4 : mode == 2 ? 8 : 0);
            allowance = failure;
            FVM_Result result = fvm_object_call(c, array, method, mode == 3 ? 2 : 1, args, &out);
            allowance = -1;
            assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
            succeeded = result == FVM_OK;
            if (!succeeded) ++failures;
            assert(c->gc_frames == 0 && c->gc_native_depth == 0 && c->avm1_execution_depth == 0 && c->string_depth == 0);
            assert(c->calling_setter_owner == 0 && c->calling_setter_name == 0);
            if (succeeded && mode == 2) assert(fvm_gc_pin(c, out.object) == FVM_OK);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            for (unsigned i = 0; i < 4; ++i) {
                int actual = rank(c, array, i, mode == 3);
                if (mode == 2) assert(actual == initial[i]);
                else if (succeeded) assert(actual == (int)i + 1);
                else assert(actual == initial[i] || actual == (int)i + 1);
                if (succeeded && mode == 2) assert(rank(c, out.object, i, false) == indices[i]);
            }
            assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.number == 4);
            fvm_context_destroy(c); assert(active_allocations == baseline);
        }
        assert(succeeded && failures);
    }
    fvm_runtime_destroy(rt); assert(active_allocations == 0);
}
