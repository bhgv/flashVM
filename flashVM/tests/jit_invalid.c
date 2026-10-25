/* Invalid/unsupported native inputs preserve handles and never replace an interpreted function. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/jit.h"


int main(void)
{
    const unsigned char bad_waits[][12] = {
        {0x8a, 2, 0, 0, 0, 0}, {0x8d, 0, 0, 0}, {0x99, 2, 0, 7, 0, 0x8a, 3, 0, 0x81, 0x3e, 255, 0}};
    const unsigned int wait_sizes[] = {6, 4, 12};
    const unsigned char bad_dispatch[][10] = {{0x83, 1, 0, 'u', 0},
                                              {0x83, 2, 0, 'u', 0, 0},
                                              {0x83, 3, 0, 0, 0, 0, 0},
                                              {0x9a, 0, 0, 0},
                                              {0x9a, 1, 0, 3, 0},
                                              {0x9a, 1, 0, 4, 0},
                                              {0x9e, 1, 0, 0, 0}};
    const unsigned int dispatch_sizes[] = {5, 6, 7, 4, 5, 5, 5};
    const unsigned char good[] = {0};
    const unsigned char unsupported[] = {0x16, 0};
    const unsigned char short_record[] = {0x96, 5, 0, 7};
    const unsigned char bad_string[] = {0x96, 2, 0, 0, 'x'};
    const unsigned char bad_target[] = {0x99, 2, 0, 1, 0, 0x96, 5, 0, 7, 1, 0, 0, 0, 0};
    const unsigned char bad_branch[] = {0x99, 1, 0, 0, 0};
    FVM_JitCode *code = 0, *keep;
    const unsigned char short_float[] = {0x96, 4, 0, 1, 0, 0, 0};
    const unsigned char short_double[] = {0x96, 8, 0, 6, 0, 0, 0, 0, 0, 0, 0};
    const unsigned char short_register[] = {0x96, 1, 0, 4};
    const unsigned char long_store[] = {0x87, 2, 0, 0, 0};
    const unsigned char bad_frames[][8] = {{0x81, 1, 0, 0, 0},
                                           {0x81, 3, 0, 0, 0, 0, 0},
                                           {0x8c, 0, 0, 0},
                                           {0x8c, 1, 0, 'x', 0},
                                           {0x8c, 3, 0, 'x', 0, 0, 0},
                                           {0x9f, 0, 0, 0},
                                           {0x9f, 1, 0, 4, 0},
                                           {0x9f, 1, 0, 2, 0},
                                           {0x9f, 3, 0, 0, 0, 0, 0}};
    const unsigned int bad_sizes[] = {5, 7, 4, 5, 7, 4, 5, 5, 7};
    unsigned int i;
    if (!(fvm_jit_compile(0, 1, &code) == FVM_INVALID_ARGUMENT && !code &&
          fvm_jit_compile(good, 0, &code) == FVM_INVALID_ARGUMENT && !code &&
          fvm_jit_compile(good, 1, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!fvm_jit_available())
    {
        return 0;
    }
    if (!(fvm_jit_compile(good, sizeof(good), &code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    keep = code;
    if (!(fvm_jit_compile(unsupported, sizeof(unsupported), &code) == FVM_UNSUPPORTED && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(short_record, sizeof(short_record), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(bad_string, sizeof(bad_string), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(bad_target, sizeof(bad_target), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(bad_branch, sizeof(bad_branch), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(short_float, sizeof(short_float), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(short_double, sizeof(short_double), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(short_register, sizeof(short_register), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(long_store, sizeof(long_store), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < sizeof(bad_sizes) / sizeof(bad_sizes[0]); ++i)
    {
        if (!(fvm_jit_compile(bad_frames[i], bad_sizes[i], &code) == FVM_INVALID_ARGUMENT && code == keep))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < sizeof(wait_sizes) / sizeof(wait_sizes[0]); ++i)
    {
        if (!(fvm_jit_compile(bad_waits[i], wait_sizes[i], &code) == FVM_INVALID_ARGUMENT && code == keep))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < sizeof(dispatch_sizes) / sizeof(dispatch_sizes[0]); ++i)
    {
        if (!(fvm_jit_compile(bad_dispatch[i], dispatch_sizes[i], &code) == FVM_INVALID_ARGUMENT && code == keep))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
