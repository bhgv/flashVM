#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cstdlib>
#include <new>

static long allowance = -1;
static long active_allocations = 0;


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

#include "../src/core/crash_internal.hpp"
#include "flashvm/exception.h"
#include <cstring>

static unsigned callbacks;


static void paused(FVM_Context *c, FVM_Result result, void *)
{
    ++callbacks;
    if (!(result == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    if (!(fvm_crash_argument(c, 0, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value arg;
    if (!(fvm_value_string_bytes(&arg, "a\0b", 3) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char bytes[] = {0};


    fvm_detail::string name("oom");
    if (!(fvm_crash_configure(c, FVM_CRASH_DEBUGGER, 0, paused, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    long baseline = active_allocations;
    allowance = 0;
    for (unsigned i = 0; i < 100; ++i)
    {


        FVM_CrashScope scope(c, &name, bytes, sizeof(bytes), 1, &arg);
    }
    allowance = -1;
    if (!(active_allocations == baseline && !c->crash.active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_set_detail(c, FVM_CRASH_DETAIL_FORCE) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool success = false;
    unsigned failures = 0;
    for (long n = 0; n < 512 && !success; ++n)
    {
        if (!(fvm_crash_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        callbacks = 0;
        FVM_GCRootFrame *roots = c->gc_frames;
        allowance = n;
        {


            FVM_CrashScope scope(c, &name, bytes, sizeof(bytes), 1, &arg);
            fvm_crash_failure(c, FVM_EXCEPTION);
        }
        allowance = -1;
        if (!(callbacks == 1 && !c->crash.active && c->gc_frames == roots && !c->crash.busy))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t count;
        FVM_Result original = FVM_EXCEPTION, diagnostic;
        FVM_Result result = fvm_crash_info(c, &count, &original, &diagnostic);
        if (!(original == FVM_EXCEPTION && (result == FVM_OK || result == FVM_NOT_FOUND)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        success = diagnostic == FVM_OK;
        if (!success)
        {
            ++failures;
            if (!(diagnostic == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(success && failures && fvm_crash_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Forced object arguments retain detached graphs across an actual collection. */
    FVM_Object *object;
    if (!(fvm_object_create(c, &object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = object;
    {


        FVM_CrashScope scope(c, &name, bytes, sizeof(bytes), 1, &arg);
        fvm_value_undefined(&arg);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bool found = false;
        for (size_t i = 0; i < c->objects.size(); ++i)
        {
            if (c->objects[i] == object)
            {
                found = true;
            }
        }
        if (!(found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
