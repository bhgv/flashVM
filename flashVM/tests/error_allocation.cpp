#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/mutex.h"
#include "flashvm/property.h"
#include "flashvm/metamethod.h"
#include <cstdlib>
#include <new>
#include <cstdio>

static long allowance = -1;
static long allocation_count = 0;


/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    ++allocation_count;
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


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *baseline = 0;
    if (!(fvm_context_create(runtime, &baseline) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    long before_install = allocation_count;
    if (!(fvm_standard_install(baseline) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    long install_allocations = allocation_count - before_install;
    if (!(install_allocations > 0 && install_allocations < 20000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(baseline);
    bool installed = false, constructed = false;
    for (long failure = 0; failure <= install_allocations && !installed; ++failure)
    {
        FVM_Context *c = 0;
        FVM_Value value;
        if (!(fvm_context_create(runtime, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = failure;
        FVM_Result result = fvm_standard_install(c);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (result == FVM_OK)
        {
            installed = true;
        }
        else
        {
            const char *names[] = {"Math",
                                   "Boolean",
                                   "Number",
                                   "Error",
                                   "parseInt",
                                   "parseFloat",
                                   "String",
                                   "NaN",
                                   "Infinity",
                                   "Object",
                                   "Array",
                                   "Date",
                                   "Function",
                                   "MovieClip",
                                   "ASSetPropFlags",
                                   "flash"};
            for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
            {
                if (!(fvm_get_global(c, names[i], &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
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
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    if (!(installed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *constructor_names[] = {
        "Error", "Number", "String", "Object", "Array", "Date", "Function", "MovieClip", "Boolean"};
    for (unsigned int constructor_index = 0;
         constructor_index < sizeof(constructor_names) / sizeof(constructor_names[0]);
         ++constructor_index)
    {
        constructed = false;
        for (long failure = 0; failure < 512 && !constructed; ++failure)
        {
            FVM_Context *c = 0;
            FVM_Object *error = 0;
            FVM_Value message, text;
            if (!(fvm_context_create(runtime, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&message, "allocation checkpoint");
            unsigned int argument_count = 1;
            if (constructor_index == 3 || constructor_index == 6 || constructor_index == 7)
            {
                argument_count = 0;
            }
            if (constructor_index == 4 || constructor_index == 5)
            {
                fvm_value_number(&message, 3);
            }
            allowance = failure;
            FVM_Result result =
                fvm_construct(c, constructor_names[constructor_index], argument_count, &message, &error);
            allowance = -1;
            if (result != FVM_OK && result != FVM_OUT_OF_MEMORY)
            {
                std::fprintf(stderr,
                             "constructor %s allocation %ld returned %d\n",
                             constructor_names[constructor_index],
                             failure,
                             (int)result);
            }
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (result == FVM_OK)
            {
                constructed = true;
                if (!(error))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, error, "toString", 0, 0, &text) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(!error))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, constructor_names[constructor_index], argument_count, &message, &error) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
        if (!(constructed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    bool materialized = false;
    for (long failure = 0; failure < 256 && !materialized; ++failure)
    {
        FVM_Context *c = 0;
        FVM_Mutex *mutex = 0;
        FVM_Value error, code;
        if (!(fvm_context_create(runtime, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_mutex_create(c, &mutex) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_mutex_close(mutex) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_mutex_set(c, mutex, 0) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = failure;
        FVM_Result result = fvm_exception_get(c, &error);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (result == FVM_OK)
        {
            materialized = true;
        }
        else
        {
            if (!(error.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_get(c, &error) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_get(c, error.object, "code", &code) == FVM_OK && code.number == 1001))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_mutex_release(mutex);
        fvm_context_destroy(c);
    }
    if (!(materialized))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *parsers[] = {"parseInt", "parseFloat", "String"};
    for (unsigned int parser = 0; parser < 3; ++parser)
    {
        FVM_Context *c = 0;
        FVM_Value input, output;
        if (!(fvm_context_create(runtime, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&input, "123.5tail");
        bool completed = false;
        for (long failure = 0; failure < 64 && !completed; ++failure)
        {
            allowance = failure;
            FVM_Result result = fvm_call(c, parsers[parser], 1, &input, &output);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            completed = result == FVM_OK;
            if (!completed)
            {
                if (!(output.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(completed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    {
        FVM_Context *c = 0;
        FVM_Object *string = 0;
        FVM_Value input, output;
        if (!(fvm_context_create(runtime, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&input, "allocation checkpoint");
        if (!(fvm_construct(c, "String", 1, &input, &string) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *methods[] = {"charAt", "substring", "indexOf", "concat"};
        for (unsigned int method_index = 0; method_index < 4; ++method_index)
        {
            bool completed = false;
            for (long failure = 0; failure < 128 && !completed; ++failure)
            {
                allowance = failure;
                FVM_Result result = fvm_object_call(c, string, methods[method_index], 0, 0, &output);
                allowance = -1;
                if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                completed = result == FVM_OK;
                if (!completed)
                {
                    if (!(output.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (!(completed))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        bool completed = false;
        for (long failure = 0; failure < 64 && !completed; ++failure)
        {
            allowance = failure;
            FVM_Result result = fvm_object_get(c, string, "length", &output);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            completed = result == FVM_OK;
            if (!completed)
            {
                if (!(output.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(completed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    {
        FVM_Context *c = 0;
        FVM_Object *object = 0;
        FVM_Value value;
        if (!(fvm_context_create(runtime, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 9);
        if (!(fvm_object_set(c, object, "a_long_property_name_to_require_allocation", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bool completed = false;
        for (long failure = 0; failure < 512 && !completed; ++failure)
        {
            FVM_Object *keys = 0;
            allowance = failure;
            FVM_Result result = fvm_object_enumerate(c, object, 1, &keys);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            completed = result == FVM_OK;
            if (!completed)
            {
                if (!(!keys))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(completed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        completed = false;
        for (long failure = 0; failure < 64 && !completed; ++failure)
        {
            FVM_Value key, entry;
            allowance = failure;
            FVM_Result result = fvm_meta_iterate(c, object, 0, &key, &entry);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            completed = result == FVM_OK;
            if (!completed)
            {
                if (!(key.type == FVM_VALUE_UNDEFINED && entry.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(completed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    {
        FVM_Context *c = 0;
        FVM_Value out;
        const unsigned short units[] = {65, 0, 0xdfff};
        bool completed = false;
        if (!(fvm_context_create(runtime, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (long failure = 0; failure < 128 && !completed; ++failure)
        {
            allowance = failure;
            FVM_Result result = fvm_value_string_utf16(c, units, 3, &out);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            completed = result == FVM_OK;
            if (!completed)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(completed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
}
