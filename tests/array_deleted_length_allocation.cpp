/* Recreated length writes propagate OOM and retry without changing readonly raw length. */
#define main array_deleted_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
#include "array_deleted_length.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"join", "slice", "push", "push"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned method = 0; method < 4; ++method) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Value raw, out, input; unsigned count = 0; FVM_Object *array = array_deleted_fixture(c, method == 3 ? 1 : 2, raw, count), *meta = 0;
            assert(fvm_gc_pin(c, array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK && fvm_gc_pin(c, meta) == FVM_OK);
            FVM_Value target = class_value(array); assert(fvm_object_set(c, meta, "cycle", &target) == FVM_OK);
            if (method != 3) assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
            fvm_value_string_bytes(&input, "r\0s", 3); input.metatable = meta;
            allowance = boundary; FVM_Result status = fvm_object_call(c, array, methods[method], method >= 2 ? 1 : 0, &input, &out); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) { failed = true; if (method == 3) { int removed = 0; assert(fvm_object_delete(c, array, "length", &removed) == FVM_OK); } assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_call(c, array, methods[method], method >= 2 ? 1 : 0, &input, &out) == FVM_OK); }
            else { assert(status == FVM_OK); success = true; }
            if (!method) assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "11"));
            if (method == 1) { assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &raw) == FVM_OK && raw.number == 1); }
            if (method >= 2) { assert(out.type == FVM_VALUE_NUMBER && out.number == (method == 3 ? 1 : 2)); assert(fvm_gc_unpin(c, meta) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK); assert(fvm_table_get_index(c, array, method == 3 ? 0 : 1, &out) == FVM_OK && out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "r\0s", 3) && out.metatable == meta); }
            assert(fvm_object_get(c, array, "length", &raw) == FVM_OK);
            if (method == 3) assert(raw.type == FVM_VALUE_NUMBER && raw.number == 1);
            else assert(raw.type == FVM_VALUE_STRING && !std::strcmp(raw.string, "1"));
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
