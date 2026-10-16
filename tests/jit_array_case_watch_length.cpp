/* Native SetMember agrees with interpreted raw Array-length watcher and resize. */
#define main array_case_watch_length_fixture_main
#include "array_case_watch_length.cpp"
#undef main
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *fn = 0; FVM_Value value, args[3], out; const unsigned char end[] = {0};
        fvm_value_number(&value, 3); assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK);
        value = class_value(array); assert(fvm_set_global(c, "caseArray", &value) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK && fvm_function_set_body(c, fn, length_watch, 0, 0) == FVM_OK);
        fvm_value_string(args, "length"); args[1] = class_value(fn); fvm_value_number(args + 2, 42);
        assert(fvm_object_call(c, array, "watch", 3, args, &out) == FVM_OK && out.boolean);
        Program p; p.variable("caseArray"); p.text("LENGTH"); p.number(5); p.op(0x4f); p.number(99); p.finish();
        length_watch_calls = 0; out = class_run(c, p); assert(out.number == 99);
        assert(length_watch_calls == (version <= 6 ? 1U : 0U));
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == (version <= 6 ? 4 : 3));
        assert(c->watch_running.empty() && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
