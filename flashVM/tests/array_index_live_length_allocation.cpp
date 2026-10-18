/* OOM at live indexed growth can be retried and preserves owned counted input metadata. */
#define main index_live_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_INDEX_LIVE_LENGTH_FIXTURE_ONLY
#include "array_index_live_length.cpp"
static FVM_Result index_live_budget(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Value value; fvm_value_number(&value, 64);
    assert(fvm_object_set(c, frame->capture, "length", &value) == FVM_OK);
    /* Leave less quota than the raw numeric tail; a reset between conversion and deletion is wrong. */
    while (c->avm1_steps < 49) ++c->avm1_steps;
    fvm_value_number(out, 0); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 1; kind <= 3; ++kind)
    for (unsigned route = 0; route < 2; ++route) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Value raw, input, out; unsigned count = 0; FVM_Object *array = array_deleted_fixture(c, kind, raw, count), *meta = 0;
            assert(fvm_gc_pin(c, array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK && fvm_gc_pin(c, meta) == FVM_OK);
            FVM_Value target = class_value(array); assert(fvm_object_set(c, meta, "cycle", &target) == FVM_OK);
            assert(fvm_value_string_bytes(&input, "a\0b", 3) == FVM_OK); input.metatable = meta;
            allowance = boundary; FVM_Result status = route ? fvm_table_set_index(c, array, 3, &input) : fvm_object_set(c, array, "\t+003", &input); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK); status = route ? fvm_table_set_index(c, array, 3, &input) : fvm_object_set(c, array, "\t+003", &input); assert(status == FVM_OK); }
            else { assert(status == FVM_OK); success = true; }
            assert(fvm_gc_unpin(c, meta) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, array, route ? "3" : "\t+003", &out) == FVM_OK && out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3) && out.metatable == meta);
            assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 4);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = deleted_length_array(c), *number = 0, *fn = version_function(c, 0); assert(fvm_gc_pin(c, array) == FVM_OK && fvm_construct(c, "Object", 0, 0, &number) == FVM_OK && fvm_function_set_body(c, fn, index_live_budget, 0, array) == FVM_OK);
        FVM_Value value = class_value(fn); assert(fvm_object_set(c, number, "valueOf", &value) == FVM_OK); value = class_value(number); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        rt->config.instruction_limit = 64; fvm_value_number(&value, 81);
        assert(fvm_object_set(c, array, "2147483647", &value) == FVM_ERROR); rt->config.instruction_limit = cfg.instruction_limit;
        int own = 1; assert(fvm_object_has_own(c, array, "2147483647", &own) == FVM_OK && !own && !c->avm1_execution_depth && !c->gc_frames);
        assert(fvm_object_set(c, array, "2147483647", &value) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
