/* Interpreted, attached and automatic native CallMethod share loaded apply preparation work. */
#define FVM_APPLY_WORK_FIXTURE_ONLY
#include "function_apply_work.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        unsigned long baseline = 0;
        for (class_mode = 0; class_mode < 3; ++class_mode) {
            FVM_Context *c = 0; FVM_Object *fn = 0, *source = 0; FVM_Value value, out;
            assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            assert(fvm_function_create_native(c, work_target, 0, 0, &fn) == FVM_OK);
            value = apply_object(fn); assert(fvm_set_global(c, "applyFunction", &value) == FVM_OK);
            assert(fvm_object_create(c, &source) == FVM_OK); value = apply_object(source);
            assert(fvm_set_global(c, "applyList", &value) == FVM_OK);
            fvm_value_number(&value, 3); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            Program call; call.variable("applyList"); call.op(0x96); call.op(1); call.op(0); call.op(2);
            call.number(2); call.variable("applyFunction"); call.text("apply"); call.op(0x52); call.finish();
            rt->config.instruction_limit = 0;
            out = class_run(c, call); assert(out.type == FVM_VALUE_NUMBER && out.number == 3);
            if (!class_mode) baseline = c->avm1_steps; else assert(c->avm1_steps == baseline);
            assert(baseline > 3); work_calls = 0; rt->config.instruction_limit = baseline - 2;
            class_run(c, call, FVM_ERROR); assert(!work_calls && c->avm1_steps == baseline - 2 && !c->avm1_execution_depth);
            rt->config.instruction_limit = 0;
            out = class_run(c, call); assert(out.number == 3);
            fvm_value_number(&value, 2.9); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            out = class_run(c, call); assert(out.number == 2);
            fvm_value_number(&value, 5); assert(fvm_set_global(c, "applyList", &value) == FVM_OK);
            out = class_run(c, call); assert(out.number == 0);
            assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
