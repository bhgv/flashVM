/* Number, ToNumber and numeric actions share exact legacy decimal results in native code. */
#define FVM_DECIMAL_NUMBERS_FIXTURE_ONLY
#include "avm1_decimal_numbers.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; FVM_Value out;
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        Program number; number.text(".1499999"); number.number(1); number.text("Number");
        number.op(0x3d); number.finish();
        out = class_run(c, number); assert(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005);
        Program action; action.text(".1499999"); action.op(0x4a); action.finish();
        out = class_run(c, action); assert(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005);
        Program subtract; subtract.text(".1499999"); subtract.number(0); subtract.op(0x0b); subtract.finish();
        out = class_run(c, subtract); assert(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005);
        Program malformed; malformed.text("1.2.5"); malformed.op(0x4a); malformed.finish();
        out = class_run(c, malformed); assert(out.type == FVM_VALUE_NUMBER);
        assert(version == 4 ? out.number == 1.25 : out.number != out.number);
        Program wrapped; wrapped.text("1e4294967297"); wrapped.op(0x4a); wrapped.finish();
        out = class_run(c, wrapped); assert(out.type == FVM_VALUE_NUMBER && out.number == 10);
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
