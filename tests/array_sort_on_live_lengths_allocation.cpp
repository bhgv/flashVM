/* Live signed length reader and sortOn preparation preserve error/output state across OOM. */
#define main sort_live_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_SORT_LIVE_LENGTH_FIXTURE_ONLY
#include "array_sort_on_live_lengths.cpp"
#include "../src/standard/array_generic.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned operation = 0; operation < 2; ++operation) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *fields = sort_live_list(c, false, 2), *options = sort_live_list(c, true, 1), *receiver = sort_live_receiver(c);
            assert(fvm_gc_pin(c, fields) == FVM_OK && fvm_gc_pin(c, options) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            if (!operation) { FVM_Object *number = 0; assert(fvm_construct(c, "Object", 0, 0, &number) == FVM_OK); FVM_Value value = class_value(version_function(c, 1)); assert(fvm_object_set(c, number, "valueOf", &value) == FVM_OK); value = class_value(number); assert(fvm_object_set(c, fields, "length", &value) == FVM_OK); }
            FVM_Object *original[3]; FVM_Value saved;
            for (unsigned i = 0; i < 3; ++i) { assert(fvm_table_get_index(c, receiver, i, &saved) == FVM_OK && fvm_gc_pin(c, saved.object) == FVM_OK); original[i] = saved.object; }
            long length = 77; FVM_Value args[2] = {class_value(fields),class_value(options)}, out;
            allowance = boundary; FVM_Result status = !operation ? fvm_array_legacy_length(c, fields, &length) : fvm_object_call(c, receiver, "sortOn", 2, args, &out); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; if (!operation) assert(length == 77);
                /* Sorting may publish partial writes before OOM: restore the pinned input for retry. */
                if (operation) for (unsigned i = 0; i < 3; ++i) { saved = class_value(original[i]); assert(fvm_table_set_index(c, receiver, i, &saved) == FVM_OK); }
                assert(fvm_gc_collect(c, 0) == FVM_OK);
                assert((!operation ? fvm_array_legacy_length(c, fields, &length) : fvm_object_call(c, receiver, "sortOn", 2, args, &out)) == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            if (!operation) assert(length == 1);
            else { assert(out.type == FVM_VALUE_OBJECT && out.object == receiver); for (unsigned i = 0; i < 3; ++i) { FVM_Value item; assert(fvm_table_get_index(c, receiver, i, &item) == FVM_OK && fvm_object_get(c, item.object, "key", &out) == FVM_OK && out.number == i + 1); } }
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK); version_load(c, 8); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
    FVM_Object *ordinary = 0; assert(fvm_object_create(c, &ordinary) == FVM_OK); FVM_Object *foreign = sort_live_list(other, false, 0); long length = 77;
    assert(fvm_array_legacy_length(c, ordinary, &length) == FVM_INVALID_ARGUMENT && length == 77);
    assert(fvm_array_legacy_length(c, foreign, &length) == FVM_INVALID_ARGUMENT && length == 77);
    assert(fvm_array_legacy_length(other, foreign, &length) == FVM_INVALID_ARGUMENT && length == 77);
    assert(fvm_array_legacy_length(c, sort_live_list(c, false, 0), 0) == FVM_INVALID_ARGUMENT);
    fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
