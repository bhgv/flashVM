/* Typed length assignment and graph restoration recover from every injected allocation boundary. */
#define main typed_length_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_TYPED_LENGTH_FIXTURE_ONLY
#include "array_typed_length.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned kind = 0; kind < 9; ++kind) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, 8); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Value initial[3], three; for (unsigned j = 0; j < 3; ++j) fvm_value_number(initial + j, j + 10); fvm_value_number(&three, 3);
        FVM_Object *array = 0, *meta = 0; assert(fvm_construct(c, "Array", 3, initial, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK);
        FVM_Value input = typed_length_input(c, kind, meta), root = class_value(array), out;
        assert(fvm_set_global(c, "typedLengthInput", &input) == FVM_OK);
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            assert(fvm_object_set(c, array, "length", &three) == FVM_OK);
            for (unsigned j = 0; j < 3; ++j) assert(fvm_table_set_index(c, array, j, initial + j) == FVM_OK);
            allowance = boundary; FVM_Result result = fvm_object_set(c, array, "length", &input); allowance = -1;
            if (result == FVM_OUT_OF_MEMORY) {
                failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_set(c, array, "length", &input) == FVM_OK);
            } else { assert(result == FVM_OK); success = true; }
            assert(fvm_object_get(c, array, "length", &out) == FVM_OK); typed_length_equal(out, input);
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner);
        }
        assert(failed && success);
        /* Isolate the new payload allocation paths from the already-tested builtin library graph. */
        array->prototype = 0; array->properties.clear();
        if (input.type == FVM_VALUE_OBJECT) {
            input.object->prototype = 0; input.object->properties.clear();
            FVM_Value edge = class_value(meta); assert(fvm_object_set(c, input.object, "edge", &edge) == FVM_OK);
        }
        unsigned char *wire = 0; size_t bytes = 0; assert(fvm_value_serialize(c, &root, &wire, &bytes) == FVM_OK);
        failed = success = false;
        for (long boundary = 0; boundary < 4096 && !success; ++boundary) {
            FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
            unsigned long before = 0, after = 0; assert(fvm_gc_object_count(other, &before) == FVM_OK);
            allowance = boundary; FVM_Result result = fvm_value_deserialize(other, wire, bytes, &out); allowance = -1;
            if (result == FVM_OUT_OF_MEMORY) {
                failed = true; assert(out.type == FVM_VALUE_UNDEFINED && fvm_gc_object_count(other, &after) == FVM_OK && before == after);
                assert(fvm_gc_collect(other, 0) == FVM_OK && fvm_value_deserialize(other, wire, bytes, &out) == FVM_OK);
            } else { assert(result == FVM_OK); success = true; }
            FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK && fvm_object_get(other, copy, "length", &out) == FVM_OK);
            typed_length_equal(out, input);
            assert(!other->gc_frames && !other->gc_native_depth); fvm_context_destroy(other);
        }
        assert(failed && success); fvm_serialized_free(wire); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
