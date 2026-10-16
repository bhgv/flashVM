/* Loaded super writes dispatch parent virtuals without mutating ordinary storage. */
#include "property_version_fixture.hpp"
#include "../src/standard/super.hpp"
#include "../src/standard/base_state.hpp"
static unsigned super_write_calls;
static FVM_Object *super_write_receiver;
static double super_write_length;
static bool super_write_array;
static FVM_Result super_write_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{ fvm_value_number(out, 17); return FVM_OK; }
static FVM_Result super_write_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++super_write_calls; assert(frame->receiver == super_write_receiver && frame->argc == 1 && frame->argv[0].number == 81);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    if (super_write_array) { FVM_Value length; assert(fvm_object_get(c, frame->receiver, "length", &length) == FVM_OK && length.number == super_write_length); }
    fvm_value_undefined(out); return FVM_OK;
}
static FVM_Result super_write_method(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ fvm_value_number(out, 99); return FVM_OK; }
static void super_write_property(FVM_Context *c, FVM_Object *owner, const char *key, size_t size, FVM_Object *get, FVM_Object *set)
{
    FVM_Value args[3], out; fvm_value_string_bytes(args, key, size); args[1] = class_value(get);
    fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL; if (set) args[2] = class_value(set);
    assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
}
static FVM_Object *super_write_escape(FVM_Context *c, FVM_Object *receiver)
{
    Program escape; escape.variable("super"); escape.finish(); FVM_Object *function = 0; FVM_Value value, out;
    assert(fvm_function_create_avm1(c, escape.bytes.begin(), escape.bytes.size(), 0, 0, 0, &function) == FVM_OK);
    value = class_value(function); assert(fvm_object_set(c, receiver, "escape", &value) == FVM_OK);
    assert(fvm_object_call(c, receiver, "escape", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER);
    return out.object;
}
#ifndef FVM_SUPER_WRITE_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"slot", "5", "+5", "n\0m"}; const size_t sizes[] = {4, 1, 2, 3};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array)
    for (unsigned key = 0; key < 4; ++key)
    for (unsigned own = 0; own < 5; ++own)
    for (unsigned far_kind = 0; far_kind < 4; ++far_kind)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        if (key == 3 && own == 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *middle = 0, *far = 0, *get = 0, *set = 0;
        assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &middle) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, super_write_get, 0, 0) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, super_write_set, 0, 0) == FVM_OK);
        FVM_Value value, out; fvm_value_number(&value, 9);
        assert(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK);
        if (far_kind < 3) super_write_property(c, far, keys[key], sizes[key], get, far_kind == 1 ? 0 : set);
        if (far_kind == 2) far->attributes[fvm_detail::string(keys[key], sizes[key])] = version_masks[version - 4];
        if (own == 1 || own == 2) assert(fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value) == FVM_OK);
        if (own == 2) receiver->attributes[fvm_detail::string(keys[key], sizes[key])] = version_masks[version - 4] | 4;
        if (own == 3) super_write_property(c, receiver, keys[key], sizes[key], get, set);
        if (own == 4) assert(fvm_object_define_method_rooted(c, receiver, keys[key], super_write_method, 0, 0) == FVM_OK);
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, middle, far) == FVM_OK && fvm_object_set_prototype(c, receiver, middle) == FVM_OK);
        FVM_Object *facade = super_write_escape(c, receiver); value = class_value(facade); assert(fvm_set_global(c, "superWriteTarget", &value) == FVM_OK);
        fvm_value_string_bytes(&value, keys[key], sizes[key]); assert(fvm_set_global(c, "superWriteKey", &value) == FVM_OK);
        super_write_calls = 0; super_write_receiver = receiver; super_write_array = array != 0;
        super_write_length = array && (key == 1 || key == 2) && (own == 1 || own == 2) ? 6 : 0;
        fvm_value_number(&value, 81);
        if (!route) assert(fvm_object_set_bytes(c, facade, keys[key], sizes[key], &value) == FVM_OK);
        else { Program code; code.variable("superWriteTarget"); code.variable("superWriteKey"); code.number(81); code.op(0x4f); code.number(1); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        assert(super_write_calls == (!own && !far_kind ? 1U : 0U));
        int present = 0; assert(fvm_object_has_own_bytes(c, receiver, keys[key], sizes[key], &present) == FVM_OK && !!present == !!own);
        assert(facade->properties.empty() && facade->indexed.empty());
        if (array) assert(fvm_object_get(c, receiver, "length", &out) == FVM_OK && out.number == super_write_length);
        if (own == 1 || own == 2) {
            receiver->attributes.erase(fvm_detail::string(keys[key], sizes[key]));
            assert(fvm_object_get_bytes(c, receiver, keys[key], sizes[key], &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 9);
        }
        assert(fvm_object_get_bytes(c, middle, keys[key], sizes[key], &out) == FVM_OK && out.number == 9);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty());
        fvm_context_destroy(c);
    }
    /* Unloaded embedding keeps named storage and its numeric-super rejection. */
    for (unsigned key = 0; key < 3; ++key) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *parent = 0; FVM_Value value, out;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, parent, keys[key], &value) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
        FVM_Object *facade = super_write_escape(c, receiver); fvm_value_number(&value, 81);
        assert(fvm_object_set(c, facade, keys[key], &value) == (key == 1 ? FVM_UNSUPPORTED : FVM_OK));
        assert(fvm_object_get(c, receiver, keys[key], &out) == FVM_OK && out.number == (key == 1 ? 9 : 81));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

#endif
