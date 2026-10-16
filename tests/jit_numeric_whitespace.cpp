/* Real native ToNumber agrees with interpreted Flash-only whitespace coercion. */
#define FVM_NUMERIC_WHITESPACE_FIXTURE_ONLY
#include "avm1_numeric_whitespace.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const unsigned short rejected[] = {0,11,12,0xa0,0x1680,0x2009,0x2028,0x3000,0xfeff,0xd800};
    for (unsigned version = 4; version <= 8; ++version) for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; FVM_Value input, out;
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        for (unsigned i = 0; i < sizeof(rejected)/sizeof(rejected[0]); ++i) {
            unsigned short units[] = {rejected[i], '4', '2'};
            assert(fvm_value_string_utf16(c, units, 3, &input) == FVM_OK);
            assert(fvm_set_global(c, "whitespaceInput", &input) == FVM_OK);
            Program p; p.variable("whitespaceInput"); p.op(0x4a); p.finish();
            out = class_run(c, p); assert(out.type == FVM_VALUE_NUMBER);
            assert(version == 4 ? out.number == 0 : out.number != out.number);
            Program call; call.variable("whitespaceInput"); call.number(1); call.text("parseFloat");
            call.op(0x3d); call.finish();
            out = class_run(c, call); assert(out.type == FVM_VALUE_NUMBER && out.number != out.number);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
