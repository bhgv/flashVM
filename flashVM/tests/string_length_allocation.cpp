/* Count queries validate complete WTF-8 without allocating expanded UTF-16. */
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


int main()
{
    const char bytes[] = {'A', 0, (char)0xf0, (char)0x9f, (char)0x98, (char)0x80, (char)0xed, (char)0xa0, (char)0x80};
    FVM_Value value;
    if (!(fvm_value_string_bytes(&value, bytes, sizeof(bytes)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t count = 99;
    allowance = 0;
    if (!(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_OK && count == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_string_unit_count(value, count) == FVM_OK && count == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned short output[6] = {11, 22, 33, 44, 55, 66};
    if (!(fvm_value_get_utf16(&value, output, 4, &count) == FVM_INVALID_ARGUMENT && count == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 6; ++i)
    {
        if (!(output[i] == (i + 1) * 11))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Output may overlap input; the copying path must preserve snapshot behavior. */
    allowance = -1;


    union
    {
        unsigned short units[6];
        char bytes[12];
    } overlap;


    std::memcpy(overlap.bytes, bytes, sizeof(bytes));
    if (!(fvm_value_string_bytes(&value, overlap.bytes, sizeof(bytes)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_get_utf16(&value, overlap.units, 6, &count) == FVM_OK && count == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(overlap.units[0] == 'A' && overlap.units[1] == 0 && overlap.units[2] == 0xd83d &&
          overlap.units[3] == 0xde00 && overlap.units[4] == 0xd800))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 0;
    const char invalid[] = {'a', (char)0xf0, (char)0x9f};
    /* Deliberately corrupt a public borrowed view after initial validation. */
    value.string = invalid;
    value.string_size = sizeof(invalid);
    if (!(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_INVALID_ARGUMENT && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_get_utf16(&value, output, 0, &count) == FVM_INVALID_ARGUMENT && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 6; ++i)
    {
        if (!(output[i] == (i + 1) * 11))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_value_string_bytes(&value, "", 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_OK && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 7);
    if (!(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_INVALID_ARGUMENT && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    char large[257];
    unsigned short decoded[257];
    std::memset(large, 'x', sizeof(large));
    if (!(fvm_value_string_bytes(&value, large, sizeof(large)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 1;
    if (!(fvm_value_get_utf16(&value, decoded, 257, &count) == FVM_OK && count == 257))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(allowance == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 257; ++i)
    {
        if (!(decoded[i] == 'x'))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    allowance = -1;
    return 0;
}
