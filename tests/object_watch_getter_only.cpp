/* Inherited getter-only virtual dispatch consumes a thrown watcher without creating own data. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
static unsigned getter_only_calls;
static FVM_Result getter_only(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++getter_only_calls; fvm_value_number(out, 17); return FVM_OK; }
static FVM_Result getter_only_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    assert(frame->argc == 4 && frame->argv[1].type == FVM_VALUE_UNDEFINED);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Value value; fvm_value_string_bytes(&value, "watch\0only", 10); return fvm_exception_raise(c, &value);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned own = 0; own < 2; ++own) for (unsigned numeric = 0; numeric < 2; ++numeric) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *parent = 0, *fn = 0; FVM_Value args[2], value, out; const char *key = numeric ? "0" : "slot";
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, object, parent) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, own ? object : parent, key, getter_only, 0, 0, 0) == FVM_OK);
        const unsigned char end[] = {0}; assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK && fvm_function_set_body(c, fn, getter_only_watch, 0, 0) == FVM_OK);
        fvm_value_string(args, key); fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_OBJECT; args[1].object = fn;
        assert(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean);
        getter_only_calls = 0; fvm_value_number(&value, 81);
        FVM_Result status = fvm_object_set(c, object, key, &value);
        assert(status == (own ? FVM_EXCEPTION : FVM_OK) && !getter_only_calls);
        if (own) { assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 10 && !std::memcmp(out.string, "watch\0only", 10)); assert(fvm_exception_clear(c) == FVM_OK); }
        else assert(!c->exception_pending);
        int present = 0; assert(fvm_object_has_own(c, object, key, &present) == FVM_OK && !!present == !!own);
        assert(fvm_object_get(c, object, key, &out) == FVM_OK && out.number == 17 && getter_only_calls == 1);
        assert(!c->gc_frames && !c->gc_native_depth && c->watch_running.empty()); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
