/* Primitive loaded decimal coercion must not allocate a temporary host token. */
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
#define FVM_DECIMAL_NUMBERS_FIXTURE_ONLY
#include "avm1_decimal_numbers.cpp"
#include "../src/avm1/decimal.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; FVM_Value input; double number = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        fvm_value_string(&input, ".1499999");
        allowance = 0;
        assert(fvm_avm1_number(c, &input, &number) == FVM_OK && number == 0.14999990000000005);
        assert(fvm_avm1_decimal("1.2.5", 5, false, &number) == FVM_OK && number == 1.25);
        assert(fvm_avm1_decimal("1.2.5", 5, true, &number) == FVM_OK && number != number);
        assert(fvm_avm1_decimal(0, 0, true, &number) == FVM_OK && number != number);
        assert(fvm_avm1_decimal(0, 1, true, &number) == FVM_INVALID_ARGUMENT);
        assert(fvm_avm1_decimal("1", 1, true, 0) == FVM_INVALID_ARGUMENT);
        allowance = -1;
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
