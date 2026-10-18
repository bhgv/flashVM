/* apply bounds allocation by actual capture work and unwinds every allocation failure. */
#include <cstdlib>
#include <new>
static long allowance = -1;
static size_t allocation_ceiling = (size_t)-1, largest_request;
/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (size > largest_request) largest_request = size;
    if (size > allocation_ceiling) throw std::bad_alloc();
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
#define FVM_APPLY_WORK_FIXTURE_ONLY
#include "function_apply_work.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; FVM_Object *fn = 0, *source = 0; FVM_Value value, out;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        assert(fvm_function_create_native(c, work_target, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK);
        assert(fvm_object_create(c, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
        fvm_value_number(&value, 3); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        bool success = false; unsigned failures = 0;
        for (long budget = 0; budget < 256 && !success; ++budget) {
            allowance = budget;
            FVM_Result status = work_apply(c, fn, source, &out);
            allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth &&
                   !c->calling_function && !c->calling_constructor && !c->calling_owner);
            if (status == FVM_OK) { success = true; assert(out.number == 3); }
            else { assert(status == FVM_OUT_OF_MEMORY); ++failures; }
            assert(fvm_gc_collect(c, 0) == FVM_OK);
        }
        assert(success && failures);
        fvm_value_number(&value, 2147483647.0); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        work_calls = 0; rt->config.instruction_limit = 4; allocation_ceiling = 4096; largest_request = 0;
        FVM_Result status = work_apply(c, fn, source, &out);
        allocation_ceiling = (size_t)-1;
        assert(status == FVM_ERROR && !work_calls && c->avm1_steps == 4 && largest_request <= 4096);
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        rt->config.instruction_limit = 0;
        assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    FVM_Context *host = 0; FVM_Object *fn = 0, *source = 0; FVM_Value value, out;
    assert(fvm_context_create(rt, &host) == FVM_OK && fvm_standard_install(host) == FVM_OK);
    assert(fvm_function_create_native(host, work_target, 0, 0, &fn) == FVM_OK);
    assert(fvm_object_create(host, &source) == FVM_OK);
    fvm_value_number(&value, 3); assert(fvm_object_set(host, source, "length", &value) == FVM_OK);
    rt->config.instruction_limit = 1;
    assert(work_apply(host, fn, source, &out) == FVM_OK && out.number == 3);
    fvm_context_destroy(host); fvm_runtime_destroy(rt);
}
