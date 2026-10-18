/* Signed borrowed end operations retain their distinct length and deletion rules. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *o = 0; FVM_Value value, args[4], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        fvm_value_number(&value, -3); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        for (unsigned i = 0; i < 4; ++i) fvm_value_number(args + i, i + 10);
        assert(borrow(c, array, "push", o, 4, args, &out) == FVM_OK && out.number == 1);
        number(c, o, "length", 1); number(c, o, "-3", 10); number(c, o, "-2", 11); number(c, o, "-1", 12); number(c, o, "0", 13);
        fvm_value_number(&value, -2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        assert(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 10);
        int own = 1; assert(fvm_object_has_own(c, o, "-3", &own) == FVM_OK && !own); number(c, o, "length", -2);
        assert(borrow(c, array, "shift", o, 0, 0, &out) == FVM_OK && out.number == 13);
        number(c, o, "0", 13); number(c, o, "length", -2);
        assert(borrow(c, array, "unshift", o, 4, args, &out) == FVM_OK && out.number == 2);
        number(c, o, "0", 10); number(c, o, "3", 13); number(c, o, "length", -2); number(c, o, "-2", 11);
        assert(borrow(c, array, "push", o, 0, 0, &out) == FVM_OK && out.number == -2);
        assert(fvm_object_set_property_flags(c, o, "length", 4, 0) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "-2", 4, 0) == FVM_OK);
        assert(borrow(c, array, "push", o, 1, args, &out) == FVM_OK && out.number == -1);
        number(c, o, "length", -2); number(c, o, "-2", 11);
        assert(fvm_object_set_property_flags(c, o, "length", 0, 4) == FVM_OK);
        fvm_value_number(&value, 55); assert(fvm_object_set(c, o, "-3", &value) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "-3", 2, 0) == FVM_OK);
        assert(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.number == 55);
        number(c, o, "-3", 55); number(c, o, "length", -2);
        assert(fvm_object_set_property_flags(c, o, "-3", 0, 2) == FVM_OK);
        int removed = 0; assert(fvm_object_delete(c, o, "-3", &removed) == FVM_OK && removed);
        assert(fvm_object_define_accessor_rooted(c, o, "-3", getter, 0, 0, 0) == FVM_OK);
        reads = 0;
        assert(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && reads == 0);
        fvm_value_number(&value, -2147483648.0); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        assert(borrow(c, array, "pop", o, 0, 0, &out) == FVM_UNSUPPORTED);
        assert(borrow(c, array, "shift", o, 0, 0, &out) == FVM_OK && out.number == 10);
        assert(borrow(c, array, "push", o, 1, args, &out) == FVM_OK && out.number == -2147483647.0);
        number(c, o, "-2147483648", 10);
        fvm_value_number(&value, -2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        unsigned long old_limit = rt->config.instruction_limit; rt->config.instruction_limit = 1;
        assert(borrow(c, array, "push", o, 4, args, &out) == FVM_ERROR);
        rt->config.instruction_limit = old_limit; number(c, o, "length", -2);
        fvm_value_number(&value, 4294967295.0); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        const char *methods[] = {"push", "pop", "shift", "unshift"};
        const double lengths[] = {-1, -2, -3, -3};
        for (unsigned i = 0; i < 4; ++i) {
            assert(borrow(c, array, methods[i], array, 0, 0, &out) == FVM_OK);
            if (i == 0 || i == 3) assert(out.type == FVM_VALUE_NUMBER && out.number == lengths[i]);
            else assert(out.type == FVM_VALUE_UNDEFINED);
            number(c, array, "length", lengths[i]);
        }
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
