/* Inherited indexed setters do not grow a loaded Array; own setters observe prior growth. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static unsigned index_calls, index_mode;
static double expected_length;
static FVM_Result index_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{ fvm_value_undefined(out); return FVM_OK; }
static FVM_Result index_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    assert(frame->argc == 4 && frame->argv[1].type == FVM_VALUE_UNDEFINED);
    FVM_Value error; fvm_value_number(&error, 77); return fvm_exception_raise(c, &error);
}
static FVM_Result index_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++index_calls; assert(frame->argc == 1 && !c->exception_pending);
    if (index_mode >= 2) assert(frame->argv[0].type == FVM_VALUE_UNDEFINED); else assert(frame->argv[0].number == 81);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Value length; assert(fvm_object_get(c, frame->receiver, "length", &length) == FVM_OK && length.number == expected_length);
    if (index_mode & 1) { fvm_value_number(out, 88); return fvm_exception_raise(c, out); }
    fvm_value_undefined(out); return FVM_OK;
}
static FVM_Value index_object(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (unsigned own = 0; own < 2; ++own) for (index_mode = 0; index_mode < 4; ++index_mode) {
        if ((version && version < 4) || (!version && index_mode >= 2)) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *parent = 0, *get = 0, *set = 0, *watch = 0; FVM_Value args[3], value, out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, parent, array->prototype) == FVM_OK && fvm_object_set_prototype(c, array, parent) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, index_get, 0, 0) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, index_set, 0, 0) == FVM_OK);
        fvm_value_string(args, "5"); args[1] = index_object(get); args[2] = index_object(set);
        if (!version && own) { fvm_context_destroy(c); continue; }
        assert(fvm_object_call(c, own ? array : parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        if (index_mode >= 2) {
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK && fvm_function_set_body(c, watch, index_watch, 0, 0) == FVM_OK);
            args[1] = index_object(watch); assert(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean);
        }
        index_calls = 0; expected_length = version && own ? 6 : 0; fvm_value_number(&value, 81);
        FVM_Result status = fvm_table_set_index(c, array, 5, &value);
        bool error = (index_mode & 1) || (index_mode >= 2 && own);
        assert(status == (error ? FVM_EXCEPTION : FVM_OK) && index_calls == 1);
        if (error) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == (index_mode >= 2 && own ? 77 : 88)); assert(fvm_exception_clear(c) == FVM_OK); }
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == expected_length);
        int present = 0; assert(fvm_object_has_own(c, array, "5", &present) == FVM_OK && !!present == !!own);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty());
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
