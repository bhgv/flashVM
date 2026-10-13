/* Loaded unshift moves raw own values without accessors and preserves borrowed/readonly lengths. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
static unsigned writes;
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++writes; fvm_value_undefined(out); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); cfg.instruction_limit = 64;
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *get_fn = 0, *set_fn = 0, *proto = 0;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK); assert(fvm_gc_pin(c, proto) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, 77); assert(fvm_object_set(c, proto, "1", &value) == FVM_OK);
        for (unsigned protect = 0; protect < 2; ++protect) {
            FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
            assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
            fvm_value_number(&value, 2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
            fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
            fvm_value_string(args, "0"); args[1] = object(get_fn); args[2] = object(set_fn);
            assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            assert(fvm_object_set_property_flags(c, o, "0", protect ? 6 : 2, 0) == FVM_OK);
            fvm_value_string_bytes(&value, "a\0b", 3); reads = writes = 0;
            assert(borrow(c, array, "unshift", o, 1, &value, &out) == FVM_OK && out.number == 3 && reads == 0 && writes == 0);
            number(c, o, "length", 2); number(c, o, "1", 9);
            int own = 0; assert(fvm_object_has_own(c, o, "2", &own) == FVM_OK && own);
            assert(fvm_object_get(c, o, "2", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK); assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK);
            if (protect) assert(value.type == FVM_VALUE_NUMBER && value.number == 9);
            else assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
        }
        FVM_Object *o = 0; int own = 0; fvm_value_number(&value, 2);
        assert(fvm_construct(c, "Array", 1, &value, &o) == FVM_OK);
        assert(fvm_object_call(c, o, "unshift", 0, 0, &out) == FVM_OK && out.number == 2);
        assert(fvm_object_has_own(c, o, "0", &own) == FVM_OK && own); assert(fvm_object_has_own(c, o, "1", &own) == FVM_OK && own);
        assert(fvm_object_set_property_flags(c, o, "length", 4, 0) == FVM_OK); fvm_value_number(&value, 81);
        assert(fvm_object_call(c, o, "unshift", 1, &value, &out) == FVM_OK && out.number == 3); number(c, o, "length", 2); number(c, o, "0", 81);
        assert(fvm_object_has_own(c, o, "2", &own) == FVM_OK && own);
        fvm_value_number(&value, 100); assert(fvm_construct(c, "Array", 1, &value, &o) == FVM_OK);
        assert(fvm_object_call(c, o, "unshift", 0, 0, &out) == FVM_ERROR); number(c, o, "length", 100);
        assert(c->avm1_execution_depth == 0 && c->gc_frames == 0);
        unsigned char costly[190]; size_t at = 0;
        const unsigned char unit[] = {0x96,5,0,7,1,0,0,0,0x17};
        const unsigned char tail[] = {0x96,5,0,7,30,0,0,0,0x3e,0};
        for (unsigned i = 0; i < 20; ++i) { std::memcpy(costly + at, unit, sizeof(unit)); at += sizeof(unit); }
        std::memcpy(costly + at, tail, sizeof(tail));
        FVM_Object *length_object = 0, *value_of = 0;
        assert(fvm_construct(c, "Object", 0, 0, &length_object) == FVM_OK); assert(fvm_gc_pin(c, length_object) == FVM_OK);
        assert(fvm_function_create_avm1(c, costly, sizeof(costly), 0, 0, 0, &value_of) == FVM_OK);
        value = object(value_of); assert(fvm_object_set(c, length_object, "valueOf", &value) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); value = object(length_object);
        assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        assert(borrow(c, array, "unshift", o, 0, 0, &out) == FVM_ERROR);
        assert(c->avm1_execution_depth == 0 && c->gc_frames == 0);
        assert(borrow(c, array, "shift", o, 0, 0, &out) == FVM_ERROR);
        assert(c->avm1_execution_depth == 0 && c->gc_frames == 0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
