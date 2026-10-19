/* Sweep native allocation failures: broadcaster publication and list changes remain atomic. */
#include <cstdlib>
#include <new>
static long fail_after = -1;
void *operator new(size_t size)
{
    if (fail_after == 0) throw std::bad_alloc();
    if (fail_after > 0) --fail_after;
    void *memory = std::malloc(size ? size : 1);
    if (!memory) throw std::bad_alloc();
    return memory;
}
void *operator new[](size_t size) { return ::operator new(size); }
void operator delete(void *memory) throw() { std::free(memory); }
void operator delete[](void *memory) throw() { std::free(memory); }
void *operator new(size_t size, const std::nothrow_t &) throw()
{ try { return ::operator new(size); } catch (...) { return 0; } }
void *operator new[](size_t size, const std::nothrow_t &) throw()
{ try { return ::operator new(size); } catch (...) { return 0; } }
void operator delete(void *memory, const std::nothrow_t &) throw() { std::free(memory); }
void operator delete[](void *memory, const std::nothrow_t &) throw() { std::free(memory); }

#include "../src/core/runtime.hpp"
#include "flashvm/broadcaster.h"
#include "flashvm/standard.h"
#include <cassert>
#include <cstdio>
static FVM_Value value(FVM_Object *o) {
    FVM_Value v; fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = o; return v;
}
static FVM_Result receive(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data) {
    ++*static_cast<unsigned *>(data); fvm_value_undefined(out); return FVM_OK;
}
int main() {
    for (unsigned operation = 0; operation < 6; ++operation) {
        bool completed = false; unsigned failures = 0;
        for (long allocation = 0; allocation < 1024 && !completed; ++allocation) {
            FVM_Config cfg; FVM_Runtime *r = 0; FVM_Context *c = 0;
            assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK);
            assert(fvm_context_create(r, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            FVM_Object *o = 0, *a = 0, *b = 0;
            assert(fvm_object_create(c, &o) == FVM_OK && fvm_object_create(c, &a) == FVM_OK);
            assert(fvm_object_create(c, &b) == FVM_OK);
            FVM_Value args = value(a), out, before; unsigned calls = 0;
            assert(fvm_object_define_method_rooted(c, a, "onEvent", receive, &calls, 0) == FVM_OK);
            if (operation > 1) {
                assert(fvm_broadcaster_initialize(c, o) == FVM_OK);
                assert(fvm_object_call(c, o, "addListener", 1, &args, &out) == FVM_OK);
                assert(fvm_object_get(c, o, "_listeners", &before) == FVM_OK);
            }
            if (operation == 5) assert(fvm_value_string(&args, "onEvent") == FVM_OK);
            fail_after = allocation;
            FVM_Result result;
            if (!operation) result = fvm_standard_broadcaster_install(c);
            else if (operation < 3) result = fvm_broadcaster_initialize(c, o);
            else if (operation == 5)
                result = fvm_object_call(c, o, "broadcastMessage", 1, &args, &out);
            else {
                args = value(operation == 3 ? b : a);
                result = fvm_object_call(c, o, operation == 3 ? "addListener" : "removeListener", 1, &args, &out);
            }
            fail_after = -1;
            if (result == FVM_OK) completed = true;
            else {
                if (result != FVM_OUT_OF_MEMORY) std::fprintf(stderr, "operation=%u allocation=%ld result=%d\n", operation, allocation, (int)result);
                assert(result == FVM_OUT_OF_MEMORY); ++failures;
                assert(calls == 0);
                assert(!c->gc_frames && !c->gc_native_depth);
                if (!operation) assert(c->global->properties.find("AsBroadcaster") == c->global->properties.end());
                else if (operation == 1) assert(o->methods.empty() && o->accessors.empty());
                else {
                    assert(fvm_object_get(c, o, "_listeners", &out) == FVM_OK && out.object == before.object);
                    assert(out.object->resource_result.number == 1 && out.object->indexed[0].object == a);
                }
            }
            fvm_context_destroy(c); fvm_runtime_destroy(r);
        }
        assert(completed && failures > 0);
    }
}
