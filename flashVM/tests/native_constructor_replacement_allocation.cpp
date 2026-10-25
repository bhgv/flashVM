/* Native constructor replacement preserves the old registration and capture on allocation failure. */
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

#define main replacement_fixture_main
#include "global_native_replacement.cpp"

#undef main


static FVM_Result oom_factory(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *userdata)
{
    *out = replacement_value((FVM_Object *)userdata);
    return FVM_OK;
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
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 256 && !success; ++boundary)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(runtime, &c) == FVM_OK))
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
            const char *original = "NativeRegistrationWithALongCountedIdentity";
            const char *updated = version <= 6 ? "nATIVEREGISTRATIONwITHaLONGcOUNTEDiDENTITY" : original;
            FVM_Object *old_capture = replacement_capture(c, 1), *new_capture = replacement_capture(c, 2), *result = 0;
            if (!(fvm_gc_pin(c, new_capture) == FVM_OK &&
                  fvm_register_constructor_rooted(c, original, oom_factory, old_capture, old_capture) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            size_t count = c->constructors.size();
            allowance = boundary;
            FVM_Result status = fvm_register_constructor_rooted(c, updated, oom_factory, new_capture, new_capture);
            allowance = -1;
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                if (!(c->constructors.size() == count && fvm_construct(c, original, 0, 0, &result) == FVM_OK &&
                      result == old_capture))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && replacement_alive(c, old_capture)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_register_constructor_rooted(c, updated, oom_factory, new_capture, new_capture) == FVM_OK))
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
            if (!(c->constructors.size() == count && fvm_gc_unpin(c, new_capture) == FVM_OK &&
                  fvm_gc_collect(c, 0) == FVM_OK && !replacement_alive(c, old_capture) &&
                  replacement_alive(c, new_capture)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_construct(c, original, 0, 0, &result) == FVM_OK && result == new_capture))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth))
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
    fvm_runtime_destroy(runtime);
    return 0;
}
