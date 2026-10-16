/* Allocation failures during loaded coercion must unwind the shared work/GC scopes. */
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
#define FVM_PARSE_INT_LOADED_FIXTURE_ONLY
#include "global_parse_int_loaded.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        FVM_Object *text_object = 0, *radix_object = 0;
        assert(fvm_object_create(c, &text_object) == FVM_OK && fvm_gc_pin(c, text_object) == FVM_OK);
        assert(fvm_object_create(c, &radix_object) == FVM_OK && fvm_gc_pin(c, radix_object) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, text_object, "toString", parsing_text, 0, 0) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, radix_object, "valueOf", parsing_radix, 0, 0) == FVM_OK);
        FVM_Value args[2], out;
        fvm_value_undefined(args); args[0].type = FVM_VALUE_OBJECT; args[0].object = text_object;
        fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_OBJECT; args[1].object = radix_object;
        const char *names[] = {"parseInt", "parseFloat"};
        for (unsigned operation = 0; operation < 2; ++operation) {
            bool success = false, callback_failure = false; unsigned failures = 0;
            for (long budget = 0; budget < 128 && !success; ++budget) {
                parsing_order = parsing_mode = 0; allowance = budget;
                FVM_Result status = fvm_call(c, names[operation], operation ? 1 : 2, args, &out);
                allowance = -1;
                assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth &&
                       !c->primitive_depth && !c->string_depth);
                if (status == FVM_OK) {
                    success = true; assert(out.type == FVM_VALUE_NUMBER && out.number == (operation ? 0 : 100));
                } else {
                    assert(status == FVM_OUT_OF_MEMORY); ++failures;
                    if (parsing_order) callback_failure = true;
                }
                assert(fvm_gc_collect(c, 0) == FVM_OK);
            }
            assert(success && failures && callback_failure);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
