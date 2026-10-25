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

#define FVM_CONVOLUTION_FIXTURE_ONLY
#include "convolution_filter.cpp"


static void equal_state(const FVM_Convolution &a, const FVM_Convolution &b)
{
    if (!(a.length == b.length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        if (!(a.parameters[i] == b.parameters[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < a.length; ++i)
    {
        if (!(a.matrix[i] == b.matrix[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


int main()
{
    long baseline = active_allocations;
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
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
    double original[] = {0, 0, 0, 0, 1, 0, 0, 0, 0}, weights[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
    FVM_Object *effect = convolution(c, 3, 3, original), *array = kernel_array(c, 9, weights),
               *image = bitmap(c, 2, 2, true, 0x80640000UL);
    double box[] = {0, 0, 2, 2};
    FVM_Object *rect = region_object(c, "flash.geom.Rectangle", 4, box), *point = hit_point(c, 0, 0);
    FVM_Value assignment = ref(array), dimension, args[4], constructor_args[3];
    fvm_value_number(&dimension, 5);
    filter_args(args, image, rect, point, effect);
    fvm_value_number(constructor_args, 3);
    fvm_value_number(constructor_args + 1, 3);
    constructor_args[2] = assignment;
    for (unsigned operation = 0; operation < 6; ++operation)
    {
        FVM_Convolution before;
        if (!(fvm_convolution_values(c, effect, before)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bool done = false;
        for (long failure = 0; failure < 8192 && !done; ++failure)
        {
            FVM_Value out;
            FVM_Object *created = 0;
            allowance = failure;
            FVM_Result r = operation == 0
                               ? fvm_construct(c, "flash.filters.ConvolutionFilter", 3, constructor_args, &created)
                           : operation == 1 ? fvm_object_set(c, effect, "matrix", &assignment)
                           : operation == 2 ? fvm_object_set(c, effect, "matrixX", &dimension)
                           : operation == 3 ? fvm_object_get(c, effect, "matrix", &out)
                           : operation == 4 ? fvm_object_call(c, effect, "clone", 0, 0, &out)
                                            : fvm_object_call(c, image, "applyFilter", 4, args, &out);
            allowance = -1;
            if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            done = r == FVM_OK;
            FVM_Convolution after;
            if (!(fvm_convolution_values(c, effect, after)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!done || (operation != 1 && operation != 2))
            {
                equal_state(before, after);
            }
            if (done && operation == 0)
            {
                if (!(fvm_gc_pin(c, created) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (done && (operation == 3 || operation == 4))
            {
                if (!(fvm_gc_pin(c, out.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (unsigned y = 0; y < 2; ++y)
            {
                for (unsigned x = 0; x < 2; ++x)
                {
                    if (!(read(c, image, x, y) == (operation == 5 && done ? 0x80ff0000UL : 0x80640000UL)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(done))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Convolution expected;
    if (!(fvm_convolution_values(c, effect, expected)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value = ref(effect);
    unsigned char *bytes = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &value, &bytes, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool done = false;
    for (long failure = 0; failure < 16384 && !done; ++failure)
    {
        FVM_Value out;
        allowance = failure;
        FVM_Result r = fvm_value_deserialize(c, bytes, size, &out);
        allowance = -1;
        if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        done = r == FVM_OK;
        if (done)
        {
            FVM_Convolution actual;
            if (!(fvm_convolution_values(c, out.object, actual)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            equal_state(expected, actual);
            if (!(fvm_gc_pin(c, out.object) == FVM_OK))
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
    }
    if (!(done))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(bytes);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (!(active_allocations == baseline))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
