/* Count queries validate complete WTF-8 without allocating expanded UTF-16. */
#include "flashvm/standard.h"
#include "../src/core/string_internal.hpp"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
        throw std::bad_alloc();
    if (allowance > 0)
        --allowance;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
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
    const char bytes[] = {'A', 0, (char)0xf0, (char)0x9f, (char)0x98, (char)0x80,
                          (char)0xed, (char)0xa0, (char)0x80};
    FVM_Value value;
    assert(fvm_value_string_bytes(&value, bytes, sizeof(bytes)) == FVM_OK);
    size_t count = 99;
    allowance = 0;
    assert(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_OK && count == 5);
    assert(fvm_string_unit_count(value, count) == FVM_OK && count == 5);
    unsigned short output[6] = {11, 22, 33, 44, 55, 66};
    assert(fvm_value_get_utf16(&value, output, 4, &count) == FVM_INVALID_ARGUMENT && count == 5);
    for (unsigned i = 0; i < 6; ++i) assert(output[i] == (i + 1) * 11);
    /* Output may overlap input; the copying path must preserve snapshot behavior. */
    allowance = -1;
    union { unsigned short units[6]; char bytes[12]; } overlap;
    std::memcpy(overlap.bytes, bytes, sizeof(bytes));
    assert(fvm_value_string_bytes(&value, overlap.bytes, sizeof(bytes)) == FVM_OK);
    assert(fvm_value_get_utf16(&value, overlap.units, 6, &count) == FVM_OK && count == 5);
    assert(overlap.units[0] == 'A' && overlap.units[1] == 0 && overlap.units[2] == 0xd83d &&
           overlap.units[3] == 0xde00 && overlap.units[4] == 0xd800);
    allowance = 0;
    const char invalid[] = {'a', (char)0xf0, (char)0x9f};
    /* Deliberately corrupt a public borrowed view after initial validation. */
    value.string = invalid;
    value.string_size = sizeof(invalid);
    assert(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_INVALID_ARGUMENT && count == 0);
    assert(fvm_value_get_utf16(&value, output, 0, &count) == FVM_INVALID_ARGUMENT && count == 0);
    for (unsigned i = 0; i < 6; ++i) assert(output[i] == (i + 1) * 11);
    assert(fvm_value_string_bytes(&value, "", 0) == FVM_OK);
    assert(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_OK && count == 0);
    fvm_value_number(&value, 7);
    assert(fvm_value_get_utf16(&value, 0, 0, &count) == FVM_INVALID_ARGUMENT && count == 0);
    allowance = -1;
    char large[257];
    unsigned short decoded[257];
    std::memset(large, 'x', sizeof(large));
    assert(fvm_value_string_bytes(&value, large, sizeof(large)) == FVM_OK);
    allowance = 1;
    assert(fvm_value_get_utf16(&value, decoded, 257, &count) == FVM_OK && count == 257);
    assert(allowance == 0);
    for (unsigned i = 0; i < 257; ++i) assert(decoded[i] == 'x');
    allowance = -1;
    return 0;
}
