/* Loaded apply reads own retained length, wraps Int32 and ignores primitive lists. */
#define FVM_APPLY_GC_FIXTURE_ONLY
#include "function_apply_gc.cpp"
#include <limits>
static unsigned expected_count, loaded_target_calls, length_getter_calls;
static FVM_Result count_target(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{ ++loaded_target_calls; assert(n == expected_count); fvm_value_number(out, n); return FVM_OK; }
static FVM_Result length_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ ++length_getter_calls; return FVM_ERROR; }
static FVM_Result apply_count(FVM_Context *c, FVM_Object *fn, const FVM_Value &list, unsigned count)
{
    FVM_Value args[2], out; fvm_value_undefined(args); args[0].type = FVM_VALUE_NULL; args[1] = list;
    expected_count = count; loaded_target_calls = 0;
    FVM_Result status = fvm_object_call(c, fn, "apply", 2, args, &out);
    if (status == FVM_OK) assert(loaded_target_calls == 1 && out.type == FVM_VALUE_NUMBER && out.number == count);
    else assert(!loaded_target_calls);
    assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
    return status;
}
static FVM_Object *length_source;
static unsigned length_conversion_calls, length_conversion_mode;
static FVM_Result convert_length(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++length_conversion_calls;
    FVM_Value zero; fvm_value_number(&zero, 0);
    assert(fvm_object_set(c, length_source, "length", &zero) == FVM_OK);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    if (length_conversion_mode == 1) return FVM_ERROR;
    if (length_conversion_mode == 2) { FVM_Value error; fvm_value_string_bytes(&error, "length\0failed", 13); return fvm_exception_raise(c, &error); }
    fvm_value_number(out, 2); return FVM_OK;
}
#ifndef FVM_APPLY_LOADED_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; FVM_Object *fn = 0, *source = 0; FVM_Value value, out;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        assert(fvm_function_create_native(c, count_target, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK);
        fvm_value_number(&value, 5); assert(apply_count(c, fn, value, 0) == FVM_OK);
        fvm_value_boolean(&value, 1); assert(apply_count(c, fn, value, 0) == FVM_OK);
        fvm_value_string(&value, "123"); assert(apply_count(c, fn, value, 0) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
        struct Case { double input; unsigned count; };
        Case cases[] = {{2.9,2},{-.9,0},{4294967298.0,2},{4294967296.0,0},
            {std::numeric_limits<double>::quiet_NaN(),0},{std::numeric_limits<double>::infinity(),0}};
        for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
            fvm_value_number(&value, cases[i].input); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            assert(apply_count(c, fn, apply_object(source), cases[i].count) == FVM_OK);
        }
        fvm_value_string(&value, "2tail"); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        assert(apply_count(c, fn, apply_object(source), version == 4 ? 2 : 0) == FVM_OK);
        fvm_value_number(&value, 2); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        FVM_Object *get_fn = 0; FVM_Value args[3];
        assert(fvm_function_create_native(c, length_getter, 0, 0, &get_fn) == FVM_OK);
        fvm_value_string(args, "length"); args[1] = apply_object(get_fn); fvm_value_undefined(args+2); args[2].type = FVM_VALUE_NULL;
        assert(fvm_object_call(c, source, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        length_getter_calls = 0;
        assert(apply_count(c, fn, apply_object(source), 2) == FVM_OK && !length_getter_calls);
        FVM_Object *child = 0; assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK);
        assert(fvm_object_set_prototype(c, child, source) == FVM_OK);
        assert(apply_count(c, fn, apply_object(child), 0) == FVM_OK && !length_getter_calls);
        FVM_Object *mixed = 0; assert(fvm_construct(c, "Object", 0, 0, &mixed) == FVM_OK);
        fvm_value_number(&value, 2); assert(fvm_object_set(c, mixed, "LeNgTh", &value) == FVM_OK);
        assert(apply_count(c, fn, apply_object(mixed), version < 7 ? 2 : 0) == FVM_OK);
        FVM_Object *array = 0; FVM_Value pair[2]; fvm_value_number(pair, 11); fvm_value_number(pair+1, 22);
        assert(fvm_construct(c, "Array", 2, pair, &array) == FVM_OK);
        assert(apply_count(c, fn, apply_object(array), 2) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &length_source) == FVM_OK && fvm_gc_pin(c, length_source) == FVM_OK);
        for (length_conversion_mode = 0; length_conversion_mode < 3; ++length_conversion_mode) {
            FVM_Object *conversion = 0, *body = 0; const unsigned char empty[] = {0};
            assert(fvm_construct(c, "Object", 0, 0, &conversion) == FVM_OK);
            assert(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &body) == FVM_OK);
            assert(fvm_function_set_body(c, body, convert_length, 0, 0) == FVM_OK);
            value = apply_object(body); assert(fvm_object_set(c, conversion, "valueOf", &value) == FVM_OK);
            value = apply_object(conversion); assert(fvm_object_set(c, length_source, "length", &value) == FVM_OK);
            length_conversion_calls = 0;
            FVM_Result status = apply_count(c, fn, apply_object(length_source), 2);
            assert(length_conversion_calls == 1);
            assert(status == (!length_conversion_mode ? FVM_OK : length_conversion_mode == 1 ? FVM_ERROR : FVM_EXCEPTION));
            if (length_conversion_mode == 2) {
                assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 13 && !std::memcmp(out.string, "length\0failed", 13));
                assert(fvm_exception_clear(c) == FVM_OK);
            }
            assert(apply_count(c, fn, apply_object(length_source), 0) == FVM_OK);
        }
        FVM_Object *bounded = 0; assert(fvm_construct(c, "Object", 0, 0, &bounded) == FVM_OK);
        const double rejected[] = {-1, 4294967295.0};
        for (unsigned i = 0; i < sizeof(rejected)/sizeof(rejected[0]); ++i) {
            fvm_value_number(&value, rejected[i]); assert(fvm_object_set(c, bounded, "length", &value) == FVM_OK);
            assert(apply_count(c, fn, apply_object(bounded), 0) == FVM_UNSUPPORTED);
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    FVM_Context *host = 0; FVM_Object *fn = 0; FVM_Value value;
    assert(fvm_context_create(rt, &host) == FVM_OK && fvm_standard_install(host) == FVM_OK);
    assert(fvm_function_create_native(host, count_target, 0, 0, &fn) == FVM_OK);
    fvm_value_number(&value, 5); assert(apply_count(host, fn, value, 0) == FVM_INVALID_ARGUMENT);
    fvm_context_destroy(host); fvm_runtime_destroy(rt);
}
#endif
