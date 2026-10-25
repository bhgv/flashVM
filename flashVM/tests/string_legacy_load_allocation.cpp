/* SWF load allocation failures preserve host String prototype and permit a clean retry. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cstdlib>
#include <new>

static long allowance = -1, active_allocations;


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
    ++active_allocations;
    return p;
}


void operator delete(void *p) throw()
{
    if (p)
    {
        --active_allocations;
    }
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

#include "../src/core/runtime.hpp"
#include "flashvm/property.h"
#include <cstring>


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    long runtime_allocations = active_allocations;
    bool succeeded = false;
    unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
    for (long failpoint = 0; failpoint < 4096 && !succeeded; ++failpoint)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value klass, value;
        if (!(fvm_get_global(c, "String", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *proto = value.object;
        fvm_value_string_bytes(&value, "a\0b", 3);
        if (!(fvm_object_set(c, proto, "marker", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *numeric[4];
        const char *names[] = {"Number", "Boolean", "Array", "Date"};
        for (unsigned i = 0; i < 4; ++i)
        {
            FVM_Value k, v;
            if (!(fvm_get_global(c, names[i], &k) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, k.object, "prototype", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            numeric[i] = v.object;
        }
        FVM_Value object_class, object_proto_value;
        if (!(fvm_get_global(c, "Object", &object_class) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, object_class.object, "prototype", &object_proto_value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *object_proto = object_proto_value.object;
        allowance = failpoint;
        FVM_Result result = fvm_load_memory(c, swf, sizeof(swf));
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (result == FVM_OUT_OF_MEMORY)
        {
            if (!(!c->swf_loaded && c->swf_bytes.empty() && proto->resource_kind == 26 && proto->resource_result_valid))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 4; ++i)
            {
                if (!(numeric[i]->resource_kind == (i == 3   ? 34
                                                    : i == 2 ? 28
                                                    : i      ? 20
                                                             : 24) &&
                      numeric[i]->resource_result_valid))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, numeric[i], "valueOf", 0, 0, &value) == FVM_OK &&
                      value.type == (i == 2   ? FVM_VALUE_OBJECT
                                     : i == 1 ? FVM_VALUE_BOOLEAN
                                              : FVM_VALUE_NUMBER)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (unsigned i = 0; i < 2; ++i)
            {
                unsigned int numeric_flags = 99;
                if (!(fvm_object_property_flags(c, numeric[i], "valueOf", &numeric_flags) == FVM_OK &&
                      numeric_flags == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(numeric[2]->methods.find("toLocaleString") != numeric[2]->methods.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned int method_flags = 99;
            if (!(fvm_object_property_flags(c, numeric[2], "push", &method_flags) == FVM_OK && method_flags == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_property_flags(c, proto, "charAt", &method_flags) == FVM_OK && method_flags == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_property_flags(c, object_proto, "valueOf", &method_flags) == FVM_OK && method_flags == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_property_flags(c, object_class.object, "registerClass", &method_flags) == FVM_OK &&
                  method_flags == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned int date_flags = 99;
            if (!(fvm_object_property_flags(c, numeric[3], "getTime", &date_flags) == FVM_OK && date_flags == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(proto->accessors.find("length") != proto->accessors.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(c, proto, "valueOf", 0, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
                  !fvm_value_string_size(&value)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            succeeded = true;
        }
        for (unsigned i = 0; i < 4; ++i)
        {
            if (!(numeric[i]->resource_kind == 30 && !numeric[i]->resource_result_valid))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(c, numeric[i], "valueOf", 0, 0, &value) == FVM_OK &&
                  value.type == (i == 2 ? FVM_VALUE_OBJECT : FVM_VALUE_UNDEFINED)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            unsigned int numeric_flags = 99;
            if (!(fvm_object_property_flags(c, numeric[i], "valueOf", &numeric_flags) == FVM_OK && numeric_flags == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(numeric[2]->methods.find("toLocaleString") == numeric[2]->methods.end()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned int method_flags = 99;
        if (!(fvm_object_property_flags(c, numeric[2], "push", &method_flags) == FVM_OK && method_flags == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_property_flags(c, proto, "charAt", &method_flags) == FVM_OK && method_flags == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_property_flags(c, object_proto, "valueOf", &method_flags) == FVM_OK && method_flags == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_property_flags(c, object_class.object, "registerClass", &method_flags) == FVM_OK &&
              method_flags == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned int date_flags = 99;
        if (!(fvm_object_property_flags(c, numeric[3], "getTime", &date_flags) == FVM_OK && date_flags == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->swf_loaded && c->swf_info.version == 8 && proto->resource_kind == 30 && !proto->resource_result_valid))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK && value.object == proto))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, proto, "valueOf", 0, 0, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, proto, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 &&
              !std::memcmp(value.string, "a\0b", 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK && c->gc_frames == 0 && !c->avm1_execution_depth && !c->string_depth &&
              !c->primitive_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        if (!(active_allocations == runtime_allocations))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(succeeded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    if (!(!active_allocations))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
