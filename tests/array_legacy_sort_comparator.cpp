/* Loaded sorting reverses DESCENDING results after comparison, including equal keys. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned compared;
static bool equal_keys, positive;
static unsigned undefined_compared;
static bool tracing;
static unsigned trace_count, error_after;
static const int pairs[][2] = {{3,1},{3,4},{3,2},{3,2},{3,4},{3,2},{2,1}};
static FVM_Result compare(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    assert(n == 2); ++compared;
    if (tracing) {
        assert(trace_count < 7 && a[0].number == pairs[trace_count][0] && a[1].number == pairs[trace_count][1]);
        ++trace_count;
        if (error_after && trace_count == error_after) return FVM_ERROR;
    }
    if (a[0].type == FVM_VALUE_UNDEFINED || a[1].type == FVM_VALUE_UNDEFINED) {
        ++undefined_compared;
        fvm_value_number(out, a[0].type == a[1].type ? 0 : a[0].type == FVM_VALUE_UNDEFINED ? -1 : 1);
    } else fvm_value_number(out, positive ? 1 : equal_keys ? 0 : a[0].number - a[1].number);
    return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[3], args[2], out; FVM_Object *array = 0, *fn = 0;
        fvm_value_number(values, 2); fvm_value_number(values + 1, 1); fvm_value_number(values + 2, 3);
        assert(fvm_construct(c, "Array", 3, values, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, compare, 0, 0, &fn) == FVM_OK); assert(fvm_gc_pin(c, fn) == FVM_OK);
        args[0] = object(fn); fvm_value_number(args + 1, 2); compared = 0;
        assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_OK && compared);
        number(c, array, "0", 3); number(c, array, "2", 1);
        FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        FVM_Value value; fvm_value_number(&value, 3); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        for (unsigned i = 0; i < 3; ++i) assert(fvm_table_set_index(c, o, i, values + i) == FVM_OK);
        FVM_Object *proto = 0; assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, proto, "1", getter, 0, 0, 0) == FVM_OK);
        assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
        fvm_value_number(args + 1, 10); compared = reads = 0;
        assert(borrow(c, array, "sort", o, 2, args, &out) == FVM_OK && compared && reads == 0 && out.type == FVM_VALUE_OBJECT && out.object != o);
        number(c, out.object, "0", 2); number(c, out.object, "1", 0); number(c, out.object, "2", 1);
        number(c, o, "0", 2); number(c, o, "1", 1); number(c, o, "2", 3);
        equal_keys = true;
        assert(borrow(c, array, "sort", o, 2, args, &out) == FVM_OK);
        number(c, out.object, "0", version ? 2 : 0);
        number(c, out.object, "1", 1);
        number(c, out.object, "2", version ? 0 : 2);
        fvm_value_number(args + 1, 12); compared = 0;
        assert(borrow(c, array, "sort", o, 2, args, &out) == FVM_OK);
        if (version) { assert(out.type == FVM_VALUE_OBJECT && compared == 4); number(c, out.object, "0", 0); number(c, out.object, "2", 2); }
        else assert(out.type == FVM_VALUE_NUMBER && out.number == 0);
        equal_keys = false;
        FVM_Value duplicates[2]; fvm_value_number(duplicates, 2); fvm_value_number(duplicates + 1, 2);
        FVM_Object *duplicate_array = 0; assert(fvm_construct(c, "Array", 2, duplicates, &duplicate_array) == FVM_OK);
        positive = true; compared = 0; fvm_value_number(args + 1, 4);
        assert(fvm_object_call(c, duplicate_array, "sort", 2, args, &out) == FVM_OK);
        if (version) assert(out.type == FVM_VALUE_NUMBER && out.number == 0 && compared == 1);
        else assert(out.type == FVM_VALUE_OBJECT && compared == 2);
        positive = false;
        fvm_value_undefined(&value);
        assert(fvm_table_set_index(c, array, 1, &value) == FVM_OK);
        fvm_value_number(args + 1, 0); undefined_compared = 0;
        assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_OK);
        assert((undefined_compared != 0) == (version != 0));
        assert(fvm_table_get_index(c, array, version ? 0 : 2, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
        number(c, array, version ? "1" : "0", 1);
        number(c, array, version ? "2" : "1", 3);
        fvm_value_string(args, "missing");
        assert(fvm_object_call(c, array, "sortOn", 1, args, &out) == FVM_OK);
        if (version) assert(out.type == FVM_VALUE_OBJECT && out.object == array);
        else assert(out.type == FVM_VALUE_UNDEFINED);
        assert(borrow(c, array, "sortOn", o, 1, args, &out) == FVM_OK);
        if (version) assert(out.type == FVM_VALUE_OBJECT && out.object == o);
        else assert(out.type == FVM_VALUE_UNDEFINED);
        if (version) {
            FVM_Value input[4]; const int initial[] = {3,1,4,2};
            for (unsigned i = 0; i < 4; ++i) fvm_value_number(input + i, initial[i]);
            for (unsigned mode = 0; mode < 4; ++mode) {
                assert(fvm_construct(c, "Array", 4, input, &array) == FVM_OK);
                args[0] = object(fn); fvm_value_number(args + 1, mode == 1 ? 2 : mode == 2 ? 10 : 0);
                trace_count = 0; error_after = mode == 3 ? 3 : 0; tracing = true;
                assert(fvm_object_call(c, array, "sort", 2, args, &out) == (mode == 3 ? FVM_ERROR : FVM_OK));
                tracing = false;
                assert(trace_count == (mode == 3 ? 3U : 7U));
                assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
                if (mode == 2) {
                    const int indices[] = {2,0,3,1};
                    for (unsigned i = 0; i < 4; ++i) { char key[2] = {(char)('0' + i),0}; number(c, out.object, key, indices[i]); number(c, array, key, initial[i]); }
                } else if (mode == 3) {
                    for (unsigned i = 0; i < 4; ++i) { char key[2] = {(char)('0' + i),0}; number(c, array, key, initial[i]); }
                } else { number(c, array, "0", mode == 1 ? 4 : 1); number(c, array, "3", mode == 1 ? 1 : 4); }
            }
            error_after = 0; compared = 0;
            assert(fvm_construct(c, "Array", 4, input, &array) == FVM_OK);
            unsigned long old_limit = rt->config.instruction_limit; rt->config.instruction_limit = 4;
            fvm_value_number(args + 1, 0);
            assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_ERROR && compared == 0);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            rt->config.instruction_limit = old_limit; number(c, array, "0", 3);
            FVM_Value ordered[16]; for (unsigned i = 0; i < 16; ++i) fvm_value_number(ordered + i, i);
            assert(fvm_construct(c, "Array", 16, ordered, &array) == FVM_OK);
            compared = 0; rt->config.instruction_limit = 64;
            assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_ERROR && compared > 0 && compared < 64);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            rt->config.instruction_limit = old_limit; number(c, array, "0", 0); number(c, array, "15", 15);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
