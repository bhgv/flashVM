/* Real native SetMember shares requested watcher spelling, errors and collecting self-removal. */
#define main property_case_watch_fixture_main
#include "object_property_case_watch.cpp"
#undef main
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (class_mode = 0; class_mode < 3; ++class_mode) for (watch_fault = 0; watch_fault < 4; ++watch_fault) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *fn = 0; FVM_Value args[3], out, value; const unsigned char end[] = {0};
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK);
        value = watch_object(object); assert(fvm_set_global(c, "caseReceiver", &value) == FVM_OK);
        assert(fvm_function_set_body(c, fn, case_watch, 0, 0) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, object, "MiXeD\0Name", 10, &value) == FVM_OK);
        fvm_value_string_bytes(args, "MiXeD\0Name", 10); args[1] = watch_object(fn); fvm_value_number(args + 2, 42);
        assert(fvm_object_call(c, object, "watch", 3, args, &out) == FVM_OK && out.boolean);
        fvm_value_string_bytes(&value, "mixed\0name", 10); assert(fvm_set_global(c, "caseKey", &value) == FVM_OK);
        Program p; p.variable("caseReceiver"); p.variable("caseKey"); p.number(81); p.op(0x4f); p.number(99); p.finish();
        const bool folded = version <= 6; watch_calls = 0;
        FVM_Result expected = folded && watch_fault == 1 ? FVM_ERROR : folded && watch_fault == 2 ? FVM_EXCEPTION : FVM_OK;
        out = class_run(c, p, expected);
        assert(watch_calls == (folded ? 1U : 0U));
        if (expected == FVM_OK) assert(out.number == 99);
        assert(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &value) == FVM_OK);
        if (folded && watch_fault == 2) assert(value.type == FVM_VALUE_UNDEFINED);
        else assert(value.type == FVM_VALUE_NUMBER && value.number == (folded && expected == FVM_OK ? 82 : 9));
        assert(c->watch_running.empty() && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
