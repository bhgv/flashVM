/* Native indexed SetMember preserves inherited-vs-own Array growth around throws. */
#define main inherited_setter_length_fixture_main
#include "array_inherited_setter_length.cpp"
#undef main
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned own = 0; own < 2; ++own) for (index_mode = 0; index_mode < 4; ++index_mode) for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *parent = 0, *get = 0, *set = 0, *watch = 0; FVM_Value args[3], value, out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, parent, array->prototype) == FVM_OK && fvm_object_set_prototype(c, array, parent) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, index_get, 0, 0) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, index_set, 0, 0) == FVM_OK);
        fvm_value_string(args, "5"); args[1] = index_object(get); args[2] = index_object(set);
        assert(fvm_object_call(c, own ? array : parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        if (index_mode >= 2) {
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK && fvm_function_set_body(c, watch, index_watch, 0, 0) == FVM_OK);
            args[1] = index_object(watch); assert(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean);
        }
        index_calls = 0; expected_length = version && own ? 6 : 0; fvm_value_number(&value, 81);

        value = index_object(array); assert(fvm_set_global(c, "caseArray", &value) == FVM_OK);
        Program p; p.variable("caseArray"); p.number(5); p.number(81); p.op(0x4f); p.number(99); p.finish();
        bool error = (index_mode & 1) || (index_mode >= 2 && own);
        out = class_run(c, p, error ? FVM_EXCEPTION : FVM_OK);
        assert(index_calls == 1); if (!error) assert(out.number == 99);
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == expected_length);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty());
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
