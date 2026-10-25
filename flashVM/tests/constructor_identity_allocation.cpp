/* Constructor metadata publication preserves data, virtual slots, flags and captures on allocation failure. */
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

#include "../src/standard/constructor_identity.hpp"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"

static unsigned identity_oom_calls;


static FVM_Result
identity_oom_trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *userdata)
{
    ++identity_oom_calls;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = (FVM_Object *)userdata;
    return FVM_OK;
}


static FVM_Value identity_oom_value(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}


static bool identity_oom_alive(FVM_Context *c, FVM_Object *object)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == object)
        {
            return true;
        }
    }
    return false;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 3; ++kind)
        {
            bool failed = false, success = false;
            for (long boundary = 0; boundary < 512 && !success; ++boundary)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(runtime, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
                      fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *receiver = 0, *capture = 0, *function = 0;
                const unsigned char end[] = {0};
                FVM_Value value, original[2], out;
                if (!(fvm_object_create(c, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK &&
                      fvm_object_create(c, &capture) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = identity_oom_value(capture);
                if (!(fvm_object_set(c, capture, "self", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &function) == FVM_OK &&
                      fvm_gc_pin(c, function) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *names[] = {"__constructor__", "constructor"};
                for (unsigned i = 0; i < 2; ++i)
                {
                    if (!kind)
                    {
                        if (!(fvm_object_set(c, receiver, names[i], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (kind == 1)
                    {
                        if (!(fvm_object_define_accessor_rooted(
                                  c, receiver, names[i], identity_oom_trap, identity_oom_trap, capture, capture) ==
                              FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_object_define_method_rooted(
                                  c, receiver, names[i], identity_oom_trap, capture, capture) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_object_set_property_flags(
                              c, receiver, names[i], FVM_PROPERTY_READ_ONLY | FVM_PROPERTY_DONT_DELETE, 0) == FVM_OK &&
                          fvm_object_get(c, receiver, names[i], original + i) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_value_string_bytes(&value, "A\0B", 3);
                if (!(fvm_object_set(c, receiver, "keep", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                identity_oom_calls = 0;
                allowance = boundary;
                FVM_Result status = fvm_function_define_constructor(c, receiver, function);
                allowance = -1;
                if (!(!identity_oom_calls && !c->gc_frames && !c->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OUT_OF_MEMORY)
                {
                    failed = true;
                    for (unsigned i = 0; i < 2; ++i)
                    {
                        unsigned flags = 0;
                        if (!(fvm_object_get(c, receiver, names[i], &out) == FVM_OK &&
                              out.object == original[i].object &&
                              fvm_object_property_flags(c, receiver, names[i], &flags) == FVM_OK &&
                              flags == (FVM_PROPERTY_READ_ONLY | FVM_PROPERTY_DONT_DELETE)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && identity_oom_alive(c, capture)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    identity_oom_calls = 0;
                    if (!(fvm_function_define_constructor(c, receiver, function) == FVM_OK && !identity_oom_calls))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = true;
                }
                for (unsigned i = 0; i < (version < 7 ? 2U : 1U); ++i)
                {
                    unsigned flags = 0;
                    if (!(fvm_object_get(c, receiver, names[i], &out) == FVM_OK && out.object == function &&
                          fvm_object_property_flags(c, receiver, names[i], &flags) == FVM_OK &&
                          flags == FVM_PROPERTY_DONT_ENUM))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_unpin(c, function) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
                      identity_oom_alive(c, capture) == (version >= 7)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, receiver, "keep", &out) == FVM_OK && fvm_value_string_size(&out) == 3 &&
                      !std::memcmp(out.string, "A\0B", 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
            if (!(failed && success))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
