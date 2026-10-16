/* apply retains captured values and its boxed receiver while script getters collect. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
static FVM_Object *pending_object;
static const char *pending_string;
static bool require_box, collected;
static unsigned apply_fault, target_calls;
static FVM_Value apply_object(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
static FVM_Result first(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    FVM_Value text; fvm_value_string_bytes(&text, "owned\0argument", 14);
    FVM_Result status = fvm_value_to_string(c, &text, out);
    if (status == FVM_OK) pending_string = out->string;
    return status;
}
static FVM_Result second(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    assert(fvm_object_create(c, &pending_object) == FVM_OK);
    *out = apply_object(pending_object);
    assert(fvm_object_set(c, pending_object, "self", out) == FVM_OK);
    return FVM_OK;
}
static FVM_Result third(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK); collected = true;
    bool live = false, box = !require_box;
    for (size_t i = 0; i < c->objects.size(); ++i) {
        FVM_Object *object = c->objects[i];
        if (object == pending_object) live = true;
        if (object->resource_kind == FVM_RESOURCE_NUMBER && object->resource_result_valid &&
            object->resource_result.type == FVM_VALUE_NUMBER && object->resource_result.number == 17) box = true;
    }
    bool string_live = false;
    for (size_t i = 0; i < c->strings.size(); ++i)
        if (c->strings[i].length == 14 && !std::memcmp(c->strings[i].data, "owned\0argument", 14)) {
            string_live = true; pending_string = c->strings[i].data;
        }
    /* Check live registries before dereferencing values in the pre-fix regression. */
    if (!live || !box || !string_live) return FVM_ERROR;
    if (apply_fault == 1) return FVM_ERROR;
    if (apply_fault == 2) { FVM_Value error; fvm_value_string_bytes(&error, "apply\0failure", 13); return fvm_exception_raise(c, &error); }
    fvm_value_number(out, 42); return FVM_OK;
}
static FVM_Result target(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    ++target_calls;
    assert(n == 3 && collected && a[0].type == FVM_VALUE_STRING && fvm_value_string_size(a) == 14);
    assert(!std::memcmp(a[0].string, "owned\0argument", 14));
    assert(a[1].type == FVM_VALUE_OBJECT && a[1].object == pending_object && a[2].number == 42);
    FVM_Value cycle; assert(fvm_object_get(c, pending_object, "self", &cycle) == FVM_OK && cycle.object == pending_object);
    if (require_box) assert(self->resource_kind == FVM_RESOURCE_NUMBER && self->resource_result.number == 17);
    fvm_value_number(out, n); return FVM_OK;
}
static void apply_getter(FVM_Context *c, FVM_Object *source, const char *name, FVM_ScriptBody callback)
{
    const unsigned char empty[] = {0}; FVM_Object *fn = 0; FVM_Value args[3], out;
    assert(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK);
    assert(fvm_function_set_body(c, fn, callback, 0, 0) == FVM_OK);
    fvm_value_string(args, name); args[1] = apply_object(fn); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, source, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
}
#ifndef FVM_APPLY_GC_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; FVM_Object *fn = 0, *source = 0; FVM_Value value, args[2], out;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        assert(fvm_function_create_native(c, target, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
        fvm_value_number(&value, 3); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        apply_getter(c, source, "0", first); apply_getter(c, source, "1", second); apply_getter(c, source, "2", third);
        args[1] = apply_object(source);
        for (unsigned mode = 0; mode < 2; ++mode) for (unsigned phase = 0; phase < 4; ++phase) {
            require_box = mode != 0; collected = false; apply_fault = phase == 3 ? 0 : phase; target_calls = 0;
            if (mode) fvm_value_number(args, 17); else { fvm_value_undefined(args); args[0].type = FVM_VALUE_NULL; }
            FVM_Result status = fvm_object_call(c, fn, "apply", 2, args, &out);
            assert(collected);
            if (!apply_fault) assert(status == FVM_OK && out.number == 3 && target_calls == 1);
            else {
                assert(!target_calls && status == (apply_fault == 1 ? FVM_ERROR : FVM_EXCEPTION));
                if (apply_fault == 2) {
                    assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 13 && !std::memcmp(out.string, "apply\0failure", 13));
                    assert(fvm_exception_clear(c) == FVM_OK);
                }
            }
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
