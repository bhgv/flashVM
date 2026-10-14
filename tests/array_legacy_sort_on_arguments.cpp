#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[2], value, args[2], out; FVM_Object *records[2], *fields = 0, *options = 0;
        for (unsigned i = 0; i < 2; ++i) { assert(fvm_construct(c, "Object", 0, 0, &records[i]) == FVM_OK); fvm_value_number(&value, i ? 2 : 10); assert(fvm_object_set(c, records[i], "rank", &value) == FVM_OK); values[i] = object(records[i]); }
        fvm_value_string(&value, "rank"); assert(fvm_construct(c, "Array", 1, &value, &fields) == FVM_OK);
        assert(fvm_construct(c, "Array", 0, 0, &options) == FVM_OK); fvm_value_number(&value, 4294967312.0); assert(fvm_table_set_index(c, options, 0, &value) == FVM_OK);
        FVM_Object *get_fn = 0; assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK);
        FVM_Value descriptor[3]; fvm_value_string(descriptor, "0"); descriptor[1] = object(get_fn); fvm_value_undefined(descriptor+2); descriptor[2].type = FVM_VALUE_NULL;
        assert(fvm_object_call(c, fields, "addProperty", 3, descriptor, &out) == FVM_OK && out.boolean);
        assert(fvm_object_call(c, options, "addProperty", 3, descriptor, &out) == FVM_OK && out.boolean);
        FVM_Object *mismatch = 0; assert(fvm_construct(c, "Array", 0, 0, &mismatch) == FVM_OK);
        for (unsigned mode = 0; mode < 8; ++mode) {
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
            args[0] = mode < 3 || mode == 6 ? object(fields) : value;
            if (mode >= 3 && mode != 6) fvm_value_string(args, "rank");
            if (mode == 0) args[1] = object(options);
            else if (mode == 1 || mode == 3) fvm_value_string(args+1, "16");
            else if (mode == 2 || mode == 4) args[1] = object(records[0]);
            else if (mode == 6) args[1] = object(mismatch);
            else if (mode == 7) args[1] = object(options);
            else fvm_value_number(args+1, 48.9);
            reads = 0; assert(fvm_object_call(c, array, "sortOn", 2, args, &out) == FVM_OK && out.object == array && !reads);
            assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.object == records[mode == 0 || mode == 1 || mode == 5 ? 1 : 0]);
        }
        FVM_Object *empty = 0, *array = 0; assert(fvm_construct(c, "Array", 0, 0, &empty) == FVM_OK); assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
        args[0] = object(empty); args[1] = object(records[0]); assert(fvm_object_call(c, array, "sortOn", 2, args, &out) == FVM_OK && out.object == array);
        assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.object == records[0]);
        fvm_value_number(values, 10); fvm_value_number(values+1, 2); assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
        args[0] = object(records[0]); fvm_value_number(args+1, 16); assert(fvm_object_call(c, array, "sortOn", 2, args, &out) == FVM_OK);
        assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.number == 10);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
