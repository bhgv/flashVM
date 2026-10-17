/* Typed native Array length retains data while only Int32 coercion controls operations. */
#include "property_version_fixture.hpp"
static FVM_Value typed_length_input(FVM_Context *c, unsigned kind, FVM_Object *meta)
{
    FVM_Value input;
    if (kind < 4) {
        const char *strings[] = {"1.9", "-3.7", "4294967297", "not a number"};
        fvm_value_string(&input, strings[kind]);
    } else if (kind < 6) fvm_value_boolean(&input, kind == 5);
    else if (kind < 8) { fvm_value_undefined(&input); if (kind == 6) input.type = FVM_VALUE_NULL; }
    else {
        FVM_Object *object = 0; assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK);
        FVM_Value fn = class_value(version_function(c, 1));
        assert(fvm_object_set(c, object, "valueOf", &fn) == FVM_OK);
        FVM_Value edge = class_value(meta); assert(fvm_object_set(c, object, "edge", &edge) == FVM_OK);
        return class_value(object);
    }
    input.metatable = meta;
    return input;
}
static void typed_length_equal(const FVM_Value &actual, const FVM_Value &input)
{
    assert(actual.type == input.type);
    if (input.type == FVM_VALUE_STRING) assert(fvm_value_string_size(&actual) == fvm_value_string_size(&input) && !std::memcmp(actual.string, input.string, fvm_value_string_size(&input)));
    if (input.type == FVM_VALUE_BOOLEAN) assert(actual.boolean == input.boolean);
}
#ifndef FVM_TYPED_LENGTH_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const long bounds[] = {1, -3, 1, 0, 0, 1, 0, 0, 1};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 9; ++kind)
    for (unsigned route = 0; route < 4; ++route)
    for (unsigned fixed = 0; fixed < 2; ++fixed) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Value initial[3]; for (unsigned j = 0; j < 3; ++j) fvm_value_number(initial + j, 10 + j);
        FVM_Object *array = 0, *meta = 0; assert(fvm_construct(c, "Array", 3, initial, &array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK);
        FVM_Value root = class_value(array), input = typed_length_input(c, kind, meta), out;
        assert(fvm_set_global(c, "typedLengthTarget", &root) == FVM_OK && fvm_object_set(c, meta, "cycle", &root) == FVM_OK);
        assert(fvm_object_set_property_flags(c, array, "2", FVM_PROPERTY_DONT_DELETE, 0) == FVM_OK);
        version_flags(c, array, "length", fixed ? 0x1007 : 0x1003, 65535);
        if (!route) assert(fvm_object_set(c, array, "length", &input) == FVM_OK);
        else {
            assert(fvm_set_global(c, "typedLengthInput", &input) == FVM_OK);
            Program code; code.variable("typedLengthTarget"); code.text("length"); code.variable("typedLengthInput"); code.op(0x4f); code.number(1); code.finish();
            out = boxing_run(c, code, route - 1, FVM_OK);
        }
        unsigned flags = 0; assert(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK && flags == (fixed ? 0x1007U : 3U));
        version_flags(c, array, "length", 0, 0xfff8);
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, array, "length", &out) == FVM_OK);
        if (fixed) assert(out.type == FVM_VALUE_NUMBER && out.number == 3 && !out.metatable);
        else { typed_length_equal(out, input); assert(input.type == FVM_VALUE_OBJECT ? out.object == input.object : out.metatable == meta); }
        for (unsigned j = 0; j < 3; ++j) { char key[2] = {(char)('0' + j), 0}; int own = 0; assert(fvm_object_has_own(c, array, key, &own) == FVM_OK && !!own == (j == 2 || (long)j < bounds[kind])); }
        if (!fixed) {
            assert(fvm_object_call(c, array, "join", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING && !std::strcmp(out.string, bounds[kind] > 0 ? "10" : ""));
            assert(fvm_object_get(c, array, "length", &out) == FVM_OK); typed_length_equal(out, input);
            unsigned char *wire = 0; size_t bytes = 0; assert(fvm_value_serialize(c, &root, &wire, &bytes) == FVM_OK);
            FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK); version_load(other, version); assert(fvm_standard_install(other) == FVM_OK);
            assert(fvm_value_deserialize(other, wire, bytes, &out) == FVM_OK); fvm_serialized_free(wire);
            FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK && fvm_object_get(other, copy, "length", &out) == FVM_OK); typed_length_equal(out, input);
            FVM_Object *copied_meta = out.metatable;
            if (out.type == FVM_VALUE_OBJECT) { assert(fvm_object_get(other, out.object, "edge", &out) == FVM_OK); copied_meta = out.object; }
            assert(copied_meta && fvm_object_get(other, copied_meta, "cycle", &out) == FVM_OK && out.object == copy);
            assert(fvm_object_call(other, copy, "join", 0, 0, &out) == FVM_OK && !std::strcmp(out.string, bounds[kind] > 0 ? "10" : ""));
            /* Old nonnumeric data never triggers tail deletion during a later length write. */
            assert(fvm_object_set_property_flags(c, array, "2", 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK);
            FVM_Value zero; fvm_value_number(&zero, 0); assert(fvm_object_set(c, array, "length", &zero) == FVM_OK);
            int own = 0; assert(fvm_object_has_own(c, array, "2", &own) == FVM_OK && own);
            assert(fvm_object_has_own(c, array, "0", &own) == FVM_OK && !!own == (bounds[kind] > 0));
            /* Index growth converts the transported live value and normalizes its length. */
            FVM_Value item; fvm_value_number(&item, 81);
            assert(fvm_object_set(other, copy, "3", &item) == FVM_OK && fvm_object_get(other, copy, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 4 && !out.metatable);
            fvm_context_destroy(other);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
