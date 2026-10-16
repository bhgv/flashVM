/* Watch lookup preserves requested spelling and raw folded old values under collecting failures. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
static unsigned watch_calls, watch_fault;
static FVM_Result case_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++watch_calls; assert(frame->argc == 4);
    assert(fvm_value_string_size(frame->argv) == 10 && !std::memcmp(frame->argv[0].string, "mixed\0name", 10));
    assert(frame->argv[1].type == FVM_VALUE_NUMBER && frame->argv[1].number == 9);
    assert(frame->argv[2].number == 81 && frame->argv[3].number == 42);
    if (watch_fault == 3) {
        FVM_Value key, ignored; fvm_value_string_bytes(&key, "MIXED\0NAME", 10);
        assert(fvm_object_call(c, frame->receiver, "unwatch", 1, &key, &ignored) == FVM_OK && ignored.boolean);
    }
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    if (watch_fault == 1) return FVM_ERROR;
    if (watch_fault == 2) { FVM_Value error; fvm_value_string_bytes(&error, "watch\0failed", 12); return fvm_exception_raise(c, &error); }
    fvm_value_number(out, 82); return FVM_OK;
}
static FVM_Value watch_object(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *fn = 0; const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK && fvm_function_set_body(c, fn, case_watch, 0, 0) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK);
        const bool folded = version && version <= 6;
        for (watch_fault = 0; watch_fault < 4; ++watch_fault) {
            FVM_Object *object = 0; FVM_Value value, args[3], out;
            assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
            fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, object, "MiXeD\0Name", 10, &value) == FVM_OK);
            fvm_value_string_bytes(args, "MiXeD\0Name", 10); args[1] = watch_object(fn); fvm_value_number(args + 2, 42);
            assert(fvm_object_call(c, object, "watch", 3, args, &out) == FVM_OK && out.boolean);
            if (folded) {
                fvm_value_string_bytes(args, "MIXED\0NAME", 10);
                assert(fvm_object_call(c, object, "watch", 3, args, &out) == FVM_OK && out.boolean && object->watchers.size() == 1);
            }
            watch_calls = 0; fvm_value_number(&value, 81);
            FVM_Result status = fvm_object_set_bytes(c, object, "mixed\0name", 10, &value);
            assert(watch_calls == (folded ? 1U : 0U));
            if (folded && (watch_fault == 1 || watch_fault == 2)) {
                assert(status == (watch_fault == 1 ? FVM_ERROR : FVM_EXCEPTION));
                if (watch_fault == 2) { assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 12 && !std::memcmp(out.string, "watch\0failed", 12)); assert(fvm_exception_clear(c) == FVM_OK); }
            } else assert(status == FVM_OK);
            assert(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &out) == FVM_OK && out.number == (folded && watch_fault != 1 && watch_fault != 2 ? 82 : 9));
            assert(!c->gc_frames && !c->gc_native_depth && c->watch_running.empty());
            if (watch_fault != 3 || !folded) {
                fvm_value_string_bytes(args, "MIXED\0NAME", 10);
                assert(fvm_object_call(c, object, "unwatch", 1, args, &out) == FVM_OK && !!out.boolean == folded);
            } else assert(object->watchers.empty());
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
