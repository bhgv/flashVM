/* Interpreted, attached and automatic native CallFunction share loaded numeric parsing. */
#define FVM_PARSE_INT_LOADED_FIXTURE_ONLY
#include "global_parse_int_loaded.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        unsigned long baseline[2] = {0, 0};
        for (class_mode = 0; class_mode < 3; ++class_mode) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
            rt->config.instruction_limit = 0;
            Program integer; integer.number(10); integer.text("0x100"); integer.number(2);
            integer.text("parseInt"); integer.op(0x3d); integer.finish();
            FVM_Value out = class_run(c, integer); assert(out.type == FVM_VALUE_NUMBER && out.number == 100);
            if (!class_mode) baseline[0] = c->avm1_steps; else assert(c->avm1_steps == baseline[0]);
            Program decimal; decimal.text(".1499999"); decimal.number(1); decimal.text("parseFloat");
            decimal.op(0x3d); decimal.finish();
            out = class_run(c, decimal); assert(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005);
            if (!class_mode) baseline[1] = c->avm1_steps; else assert(c->avm1_steps == baseline[1]);
            const char *names[] = {"parseInt", "parseFloat"};
            for (unsigned i = 0; i < 2; ++i) {
                Program empty; empty.number(0); empty.text(names[i]); empty.op(0x3d); empty.finish();
                out = class_run(c, empty); assert(out.type == FVM_VALUE_UNDEFINED);
                Program long_input; long_input.text("123456789012345678901234567890");
                long_input.number(1); long_input.text(names[i]); long_input.op(0x3d); long_input.finish();
                rt->config.instruction_limit = 10;
                class_run(c, long_input, FVM_ERROR);
                assert(c->avm1_steps == 10 && !c->avm1_execution_depth);
                rt->config.instruction_limit = 0;
                out = class_run(c, long_input); assert(out.type == FVM_VALUE_NUMBER && out.number > 0);
            }
            assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
