/* Inject every allocation boundary in folded display paths and verify cleanup. */
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
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
#define FVM_UNICODE_PATH_FIXTURE_ONLY
#include "movieclip_unicode_paths.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
    const unsigned char swf[] = {'F','W','S',6,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    FVM_Object *root = 0; assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK);
    FVM_Object *destination = child(c, root, "\xc3\x84", 1), *alias = 0;
    assert(fvm_construct(c, "Object", 0, 0, &alias) == FVM_OK);
    FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = alias;
    assert(fvm_object_set(c, root, "\xc4\xb0x", &value) == FVM_OK);
    value.object = destination; assert(fvm_object_set(c, alias, "\xd0\x94", &value) == FVM_OK);
    fvm_detail::string upper("\xc4\xb0"), lower("i"), scalar("\xf0\x90\x90\x80"), pair("\xed\xa0\x81\xed\xb0\x80");
    allowance = 0;
    assert(fvm_avm1_folded_name_equal(upper, lower) && fvm_avm1_folded_name_equal(scalar, pair));
    allowance = -1;
    for (unsigned mode = 0; mode < 2; ++mode) {
        bool succeeded = false; unsigned failures = 0;
        FVM_Value path; fvm_value_string(&path, mode ? "iX/\xd0\xb4" : "\xc3\xa4");
        for (long boundary = 0; boundary < 2000 && !succeeded; ++boundary) {
            FVM_Object *out = root; FVM_GCRootFrame *roots = c->gc_frames;
            allowance = boundary;
            FVM_Result status = fvm_movieclip_resolve_path(c, root, path, &out);
            allowance = -1;
            if (status == FVM_OK) { assert(out == destination); succeeded = true; }
            else { assert(status == FVM_OUT_OF_MEMORY && !out); ++failures; }
            assert(c->gc_frames == roots && c->avm1_execution_depth == 0 && c->gc_native_depth == 0);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_movieclip_resolve_path(c, root, path, &out) == FVM_OK && out == destination);
        }
        assert(succeeded && failures);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
