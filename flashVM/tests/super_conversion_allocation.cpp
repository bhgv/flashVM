/* Invocation roots work without allocation; superclass conversion OOM preserves aliases and permits retry. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "../src/core/string_internal.hpp"
#include <cstdlib>
#include <new>

static long allowance = -1;


/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
    {
        throw std::bad_alloc();
    }
    if (allowance > 0)
    {
        --allowance;
    }
    void *p = std::malloc(size ? size : 1);
    if (!p)
    {
        throw std::bad_alloc();
    }
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

#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
#include "flashvm/execution.h"
#include "flashvm/exception.h"
#include "../src/core/call_scope.hpp"

static FVM_Object *allocation_receiver, *allocation_owner;
static unsigned allocation_kind, allocation_throw;


static FVM_Result allocated_conversion(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == allocation_receiver && c->calling_owner == allocation_owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (allocation_throw)
    {
        fvm_value_number(out, 81);
        return fvm_exception_raise(c, out);
    }
    if (allocation_kind == 1)
    {
        return fvm_value_string_bytes(out, "r\0s", 3);
    }
    fvm_value_number(out, 9);
    return FVM_OK;
}


static bool root_alive(FVM_Context *c, FVM_Object *o)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == o)
        {
            return true;
        }
    }
    return false;
}


static void inline_call_roots(FVM_Runtime *rt)
{
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *objects[6];
    for (unsigned i = 0; i < 6; ++i)
    {
        if (!(fvm_object_create(c, objects + i) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    c->calling_constructor = objects[0];
    c->calling_function = objects[1];
    c->calling_owner = objects[2];
    bool entered = false;
    allowance = 0;
    try
    {


        FVM_CallScope scope(c, objects[3], objects[4], objects[5]);
        entered = true;
        if (!(c->calling_constructor == objects[3] && c->calling_function == objects[4] &&
              c->calling_owner == objects[5]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = -1;
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 6; ++i)
        {
            if (!(root_alive(c, objects[i])))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        allowance = 0;
        FVM_GCRootFrame *outer = c->gc_frames;
        try
        {


            FVM_CallScope nested(c, objects[0], objects[1], objects[2]);
            throw std::bad_alloc();
        }
        catch (const std::bad_alloc &)
        {
        }
        if (!(c->gc_frames == outer && c->calling_constructor == objects[3] && c->calling_function == objects[4] &&
              c->calling_owner == objects[5]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    catch (const std::bad_alloc &)
    {
    }
    allowance = -1;
    if (!(entered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_frames && c->calling_constructor == objects[0] && c->calling_function == objects[1] &&
          c->calling_owner == objects[2]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->calling_constructor = c->calling_function = c->calling_owner = 0;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 6; ++i)
    {
        if (!(!root_alive(c, objects[i])))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
}


static FVM_Result allocation_convert(FVM_Context *c, FVM_Object *facade, FVM_Value *out)
{
    FVM_Value zero;
    fvm_value_number(&zero, 0);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = facade;
    if (allocation_kind == 2)
    {
        return fvm_exec_compare(c, FVM_EXEC_ADD_DYNAMIC, out, &zero, out);
    }
    return allocation_kind == 1 ? fvm_exec_to_string(c, out, out) : fvm_exec_to_number(c, 0, out, out);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inline_call_roots(rt);
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (allocation_kind = 0; allocation_kind < 3; ++allocation_kind)
        {
            for (allocation_throw = 0; allocation_throw < 2; ++allocation_throw)
            {
                bool success = false, failed = false;
                for (long boundary = 0; boundary < 512 && !success; ++boundary)
                {
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *target = 0, *facade = 0, *method = 0;
                    super_accessor_setup(c, &allocation_receiver, &target, &facade);
                    FVM_Value base, value;
                    if (!(fvm_get_global(c, "Base", &base) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    allocation_owner = prototype(c, base);
                    const unsigned char end[] = {0};
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &method) == FVM_OK &&
                          fvm_function_set_body(c, method, allocated_conversion, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_undefined(&value);
                    value.type = FVM_VALUE_OBJECT;
                    value.object = method;
                    if (!(fvm_object_set(c, allocation_owner, allocation_kind == 1 ? "toString" : "valueOf", &value) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    allowance = boundary;
                    FVM_Result status = allocation_convert(c, facade, &value);
                    allowance = -1;
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function &&
                          !c->primitive_depth && !c->string_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (status == FVM_OUT_OF_MEMORY)
                    {
                        failed = true;
                        if (!(value.type == FVM_VALUE_OBJECT && value.object == facade))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        success = true;
                        if (!(status == (allocation_throw ? FVM_EXCEPTION : FVM_OK)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (c->exception_pending)
                    {
                        FVM_Value error;
                        if (!(allocation_throw && fvm_exception_get(c, &error) == FVM_OK && error.number == 81 &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    status = allocation_convert(c, facade, &value);
                    if (!(status == (allocation_throw ? FVM_EXCEPTION : FVM_OK)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (allocation_throw)
                    {
                        if (!(value.type == FVM_VALUE_OBJECT && value.object == facade &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (allocation_kind == 1)
                    {
                        if (!(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 &&
                              !std::memcmp(value.string, "r\0s", 3)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(value.type == FVM_VALUE_NUMBER && value.number == 9))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->calling_owner && !c->calling_function))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
                if (!(success && failed))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
