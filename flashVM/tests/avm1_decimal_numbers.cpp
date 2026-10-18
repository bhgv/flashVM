/* Number and ToNumber share AVM1 decimal accumulation with versioned strictness. */
#define FVM_PARSE_INT_LOADED_FIXTURE_ONLY
#include "global_parse_int_loaded.cpp"
#include "../src/avm1/coercion.hpp"
static void decimal_check(FVM_Context *c, const char *bytes, size_t size, double expected)
{
    FVM_Value input, out; double number = 0;
    assert(fvm_value_string_bytes(&input, bytes, size) == FVM_OK);
    assert(fvm_avm1_number(c, &input, &number) == FVM_OK);
    assert(expected != expected ? number != number : number == expected);
    assert(fvm_call(c, "Number", 1, &input, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER);
    assert(expected != expected ? out.number != out.number : out.number == expected);
    if (expected == 0) assert((1.0/out.number < 0) == (1.0/expected < 0));
}
#ifndef FVM_DECIMAL_NUMBERS_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    double nan = std::numeric_limits<double>::quiet_NaN();
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        decimal_check(c, ".1499999", 8, 0.14999990000000005);
        decimal_check(c, "-.1499999", 9, -0.14999990000000005);
        decimal_check(c, "1e4294967297", 12, 10);
        decimal_check(c, "1e-4294967297", 13, .1);
        decimal_check(c, "1e2147483648", 12, 0);
        decimal_check(c, "1e+", 3, 1);
        decimal_check(c, ".", 1, 0);
        decimal_check(c, "-.", 2, -0.0);
        decimal_check(c, "42tail", 6, version == 4 ? 42 : nan);
        decimal_check(c, "1\0more", 6, version == 4 ? 1 : nan);
        decimal_check(c, "1.2.5", 5, version == 4 ? 1.25 : nan);
        decimal_check(c, "077", 3, version < 6 ? 77 : 63);
        decimal_check(c, "0x10", 4, version == 4 ? 0 : version == 5 ? nan : 16);
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
