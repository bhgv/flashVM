/* Loaded apply reads only existing own length data, including Array lists after deletion. */
#include "property_version_fixture.hpp"
static FVM_Object *deleted_length_function(FVM_Context *c)
{
    Program body; body.variable("arguments"); body.text("length"); body.op(0x4e); body.finish();
    FVM_Object *fn = 0; assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &fn) == FVM_OK); return fn;
}
static FVM_Object *deleted_length_array(FVM_Context *c)
{
    FVM_Value pair[2]; fvm_value_number(pair, 11); fvm_value_number(pair + 1, 22);
    FVM_Object *array = 0; assert(fvm_construct(c, "Array", 2, pair, &array) == FVM_OK);
    assert(fvm_object_set_property_flags(c, array, "length", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK);
    int removed = 0; assert(fvm_object_delete(c, array, "length", &removed) == FVM_OK && removed);
    int own = 1; assert(fvm_object_has_own(c, array, "length", &own) == FVM_OK && !own); return array;
}
static FVM_Value deleted_length_invoke(FVM_Context *c, FVM_Object *array, unsigned route)
{
    FVM_Object *fn = deleted_length_function(c); FVM_Value value = class_value(fn), out;
    assert(fvm_set_global(c, "deletedLengthFunction", &value) == FVM_OK); value = class_value(array);
    assert(fvm_set_global(c, "deletedLengthList", &value) == FVM_OK);
    if (!route) { FVM_Value args[2]; fvm_value_undefined(args); args[0].type = FVM_VALUE_NULL; args[1] = value; assert(fvm_object_call(c, fn, "apply", 2, args, &out) == FVM_OK); }
    else { Program p; p.variable("deletedLengthList"); p.op(0x96); p.op(1); p.op(0); p.op(2); p.number(2); p.variable("deletedLengthFunction"); p.text("apply"); p.op(0x52); p.finish(); out = boxing_run(c, p, route - 1, FVM_OK); }
    return out;
}
#ifndef FVM_APPLY_DELETED_LENGTH_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 7; ++kind)
    for (unsigned transport = 0; transport < 2; ++transport)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0, *source = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0; FVM_Value value, out;
        if (!kind) { FVM_Value pair[2]; fvm_value_number(pair, 11); fvm_value_number(pair + 1, 22); assert(fvm_construct(c, "Array", 2, pair, &array) == FVM_OK); }
        else array = deleted_length_array(c);
        if (kind == 2 || kind == 3) {
            FVM_Object *proto = 0; assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK && fvm_object_set_prototype(c, array, proto) == FVM_OK);
            fvm_value_number(&value, 2); assert(fvm_object_set(c, proto, "length", &value) == FVM_OK);
            if (kind == 3) { Program raised; raised.number(77); raised.op(0x2a); raised.op(0); FVM_Object *getter = 0; assert(fvm_function_create_avm1(c, raised.bytes.begin(), raised.bytes.size(), 0, 0, 0, &getter) == FVM_OK); FVM_Value args[3]; fvm_value_string(args, "length"); args[1] = class_value(getter); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL; assert(fvm_object_call(c, proto, "addProperty", 3, args, &out) == FVM_OK && out.boolean); }
        }
        if (kind == 4) { fvm_value_number(&value, 2); assert(fvm_object_set(c, array, "LeNgTh", &value) == FVM_OK); }
        if (kind == 5) { fvm_value_number(&value, 2); assert(fvm_object_set_bytes(c, array, "length\0suffix", 13, &value) == FVM_OK); }
        if (kind == 6) { fvm_value_number(&value, 1); assert(fvm_object_set(c, array, "length", &value) == FVM_OK); }
        if (transport) {
            source = c; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            unsigned char *wire = 0; size_t bytes = 0; value = class_value(array);
            assert(fvm_value_serialize(source, &value, &wire, &bytes) == FVM_OK && fvm_value_deserialize(c, wire, bytes, &value) == FVM_OK); fvm_serialized_free(wire); array = value.object;
        }
        assert(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        out = deleted_length_invoke(c, array, route); const unsigned expected = !kind || (kind == 4 && version < 7) ? 2 : kind == 6 ? 1 : 0;
        assert(out.type == FVM_VALUE_NUMBER && out.number == expected);
        for (unsigned i = 0; i < 2; ++i) { assert(fvm_table_get_index(c, array, i, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER && value.number == 11 + i * 11); }
        assert(!c->exception_pending && !c->gc_frames && !c->gc_native_depth && !c->script_depth); fvm_context_destroy(c); if (source) fvm_context_destroy(source);
    }
    fvm_runtime_destroy(rt);
}
#endif
