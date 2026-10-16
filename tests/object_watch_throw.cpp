/* Loaded thrown watchers write undefined before propagating; inherited setters consume the error. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/base_state.hpp"
#include <cassert>
static FVM_Object *watch_thrown;
static unsigned setter_calls, setter_throws;
static FVM_Value thrown_object(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
static FVM_Result throwing_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    assert(frame->argc == 4 && frame->argv[1].number == 9 && frame->argv[2].number == 81);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_create(c, &watch_thrown) == FVM_OK);
    FVM_Value value = thrown_object(watch_thrown); assert(fvm_object_set(c, watch_thrown, "self", &value) == FVM_OK);
    fvm_value_number(&value, 77); assert(fvm_object_set(c, watch_thrown, "marker", &value) == FVM_OK);
    value = thrown_object(watch_thrown); return fvm_exception_raise(c, &value);
}
static FVM_Result throw_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{ fvm_value_number(out, 17); return FVM_OK; }
static FVM_Result throw_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++setter_calls; assert(frame->argc == 1 && frame->argv[0].type == FVM_VALUE_UNDEFINED && !c->exception_pending);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == watch_thrown) alive = true;
    assert(alive); FVM_Value value; assert(fvm_object_get(c, watch_thrown, "marker", &value) == FVM_OK && value.number == 77);
    if (setter_throws) { fvm_value_number(&value, 88); return fvm_exception_raise(c, &value); }
    fvm_value_undefined(out); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (unsigned kind = 0; kind < 6; ++kind) for (setter_throws = 0; setter_throws < 2; ++setter_throws) {
        if ((version && version < 4) || (!version && (kind == 3 || kind == 4))) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *owner = 0, *watch = 0, *get = 0, *set = 0; FVM_Value value, args[3], out;
        const char *key = kind == 1 || kind == 2 ? "0" : "slot";
        assert(fvm_construct(c, kind == 1 ? "Array" : "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        owner = object;
        if (kind == 4) { assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_object_set_prototype(c, object, owner) == FVM_OK); }
        fvm_value_number(&value, 9); assert(fvm_object_set(c, owner, key, &value) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK && fvm_function_set_body(c, watch, throwing_watch, 0, 0) == FVM_OK);
        if (kind == 3 || kind == 4) {
            assert(fvm_gc_pin(c, watch) == FVM_OK);
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, throw_get, 0, 0) == FVM_OK);
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, throw_set, 0, 0) == FVM_OK);
            fvm_value_string(args, key); args[1] = thrown_object(get); args[2] = thrown_object(set);
            assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        }
        if (kind == 5) assert(fvm_object_set_property_flags(c, object, key, 4, 0) == FVM_OK);
        fvm_value_string(args, key); args[1] = thrown_object(watch);
        assert(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean);
        setter_calls = 0; fvm_value_number(&value, 81);
        FVM_Result status = fvm_object_set(c, object, key, &value);
        if (!version && kind == 5) assert(status == FVM_OK);
        else if (kind == 4 && !setter_throws) assert(status == FVM_OK && !c->exception_pending);
        else {
            assert(status == FVM_EXCEPTION && fvm_exception_get(c, &out) == FVM_OK);
            if (kind == 4) assert(out.type == FVM_VALUE_NUMBER && out.number == 88);
            else assert(out.type == FVM_VALUE_OBJECT && out.object == watch_thrown);
            assert(fvm_exception_clear(c) == FVM_OK);
        }
        assert(setter_calls == (version && (kind == 3 || kind == 4) ? 1U : 0U));
        if (kind == 3 || kind == 4) {
            assert(fvm_base_accessor_data(owner->accessors.find(fvm_detail::string(key))->second, out));
            if (kind == 3) assert(out.type == FVM_VALUE_UNDEFINED || (setter_throws && out.number == 9));
            else assert(out.type == FVM_VALUE_NUMBER && out.number == 9);
        } else {
            assert(fvm_object_get(c, object, key, &out) == FVM_OK);
            if (version && kind != 5) assert(out.type == FVM_VALUE_UNDEFINED);
            else assert(out.type == FVM_VALUE_NUMBER && out.number == 9);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty());
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
