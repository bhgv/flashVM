/* Pool record widths/counts and aggregate limits reject malformed compilation without changing handles. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    const unsigned char empty[] = {0};
    FVM_JitCode *code = 0, *keep = 0;
    if (!(fvm_jit_compile(empty, sizeof(empty), &code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    keep = code;
    const unsigned char short_count[] = {0x88, 1, 0, 0};
    const unsigned char missing_text[] = {0x88, 3, 0, 1, 0, 'x'};
    const unsigned char trailing[] = {0x88, 3, 0, 0, 0, 0};
    const unsigned char short_index8[] = {0x96, 1, 0, 8};
    const unsigned char short_index16[] = {0x96, 2, 0, 9, 0};
    if (!(fvm_jit_compile(short_count, sizeof(short_count), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(missing_text, sizeof(missing_text), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(trailing, sizeof(trailing), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(short_index8, sizeof(short_index8), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(short_index16, sizeof(short_index16), &code) == FVM_INVALID_ARGUMENT && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_detail::vector<unsigned char> body;
    body.push_back(0x88);
    body.push_back(255);
    body.push_back(255);
    body.push_back(253);
    body.push_back(255);
    for (unsigned i = 0; i < 65533; ++i)
    {
        body.push_back(0);
    }
    /* 65533 empty entries plus three is exactly the aggregate limit. */
    body.push_back(0x88);
    body.push_back(5);
    body.push_back(0);
    body.push_back(3);
    body.push_back(0);
    body.push_back(0);
    body.push_back(0);
    body.push_back(0);
    body.push_back(0);
    FVM_JitCode *large = 0;
    if (!(fvm_jit_compile(body.begin(), body.size(), &large) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(large->pool_entries == 65536 && fvm_jit_destroy(large) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    body.pop_back();
    body.push_back(0x88);
    body.push_back(3);
    body.push_back(0);
    body.push_back(1);
    body.push_back(0);
    body.push_back(0);
    body.push_back(0);
    if (!(fvm_jit_compile(body.begin(), body.size(), &code) == FVM_UNSUPPORTED && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
