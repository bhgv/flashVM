/* Watch-before-growth, inherited/own virtual dispatch, and overflow budget cleanup. */
#include "property_version_fixture.hpp"
static unsigned calls;
static FVM_Result named_watch(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    assert(n == 4); ++calls; FVM_Value length; fvm_value_number(&length, -3);
    assert(fvm_object_set(c, self, "length", &length) == FVM_OK); *out = a[2]; return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *watch = 0; FVM_Value args[4], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, named_watch, 0, 0, &watch) == FVM_OK);
        fvm_value_string(args, "-1"); args[1] = class_value(watch);
        assert(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean);
        calls = 0; fvm_value_number(args, 81); assert(fvm_object_set(c, array, "-1", args) == FVM_OK && calls == 1);
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == 0);
        /* Counted NUL and non-ASCII whitespace are ordinary literal properties. */
        assert(fvm_object_set_bytes(c, array, "1\0x", 3, args) == FVM_OK);
        assert(fvm_object_set(c, array, "\xc2\xa0" "1", args) == FVM_OK);
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == 0);
        for (unsigned inherited = 0; inherited < 2; ++inherited) {
            FVM_Object *target = 0, *owner = 0, *setter = 0;
            assert(fvm_construct(c, "Array", 0, 0, &target) == FVM_OK && fvm_gc_pin(c, target) == FVM_OK);
            owner = target;
            if (inherited) { assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK); assert(fvm_object_set_prototype(c, target, owner) == FVM_OK); }
            const unsigned char body[] = {0x96,6,0,0,'t','h','i','s',0,0x1c,0x96,13,0,0,'c','a','l','l','e','d',0,7,1,0,0,0,0x4f,0};
            assert(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &setter) == FVM_OK);
            fvm_value_string(args, "+01"); args[1] = class_value(version_function(c, 42)); args[2] = class_value(setter);
            assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            fvm_value_number(args, 81); assert(fvm_object_set(c, target, "+01", args) == FVM_OK);
            assert(fvm_object_get(c, target, "length", &out) == FVM_OK && out.number == (inherited ? 0 : 2));
            assert(fvm_object_get(c, target, "called", &out) == FVM_OK && out.number == 1);
        }
        FVM_Object *budget = 0; fvm_value_number(args, 3); assert(fvm_construct(c, "Array", 1, args, &budget) == FVM_OK);
        fvm_value_number(args, 81); for (unsigned long i = 0; i < 3; ++i) assert(fvm_table_set_index(c, budget, i, args) == FVM_OK);
        const unsigned long limit = rt->config.instruction_limit; rt->config.instruction_limit = 1;
        assert(fvm_object_set(c, budget, "+2147483647", args) == FVM_ERROR); rt->config.instruction_limit = limit;
        assert(fvm_object_get(c, budget, "length", &out) == FVM_OK && out.number == 3);
        int own = 0; assert(fvm_object_has_own(c, budget, "+2147483647", &own) == FVM_OK && !own);
        assert(fvm_object_set(c, budget, "+2147483647", args) == FVM_OK);
        assert(fvm_object_get(c, budget, "length", &out) == FVM_OK && out.number == -2147483648.0);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
