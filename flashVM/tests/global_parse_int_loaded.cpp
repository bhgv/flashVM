/* Loaded parseInt follows AVM1's raw prefix, radix and conversion policies. */
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
static void parsing_load(FVM_Context *c, unsigned version)
{
    unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    swf[3] = (unsigned char)version;
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
}
static void check_parse_int(FVM_Context *c, const char *text, size_t length,
                            bool explicit_radix, double radix, double expected)
{
    FVM_Value args[2], out;
    assert(fvm_value_string_bytes(args, text, length) == FVM_OK);
    fvm_value_number(args + 1, radix);
    assert(fvm_call(c, "parseInt", explicit_radix ? 2 : 1, args, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_NUMBER);
    assert(expected != expected ? out.number != out.number : out.number == expected);
    if (expected == 0) assert((1.0 / out.number < 0) == (1.0 / expected < 0));
}
static unsigned parsing_order, parsing_mode;
static FVM_Result parsing_text(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *,
                               FVM_Value *out, void *)
{
    parsing_order = parsing_order * 10 + 1;
    assert(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
    if (parsing_mode == 3) {
        FVM_Value error; fvm_value_string(&error, "text failed");
        return fvm_exception_raise(c, &error);
    }
    if (parsing_mode == 4) { fvm_value_number(out, 42); return FVM_OK; }
    return fvm_value_string(out, "0x100");
}
static FVM_Result parsing_radix(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *,
                                FVM_Value *out, void *)
{
    parsing_order = parsing_order * 10 + 2;
    assert(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
    if (parsing_mode == 2) {
        FVM_Value error; fvm_value_string(&error, "radix failed");
        return fvm_exception_raise(c, &error);
    }
    fvm_value_number(out, parsing_mode == 1 ? 1 : 10);
    return FVM_OK;
}
#ifndef FVM_PARSE_INT_LOADED_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    struct Case { const char *text; bool explicit_radix; double radix, expected; };
    const Case cases[] = {
        {"08", false, 0, 8}, {"077", false, 0, 63}, {"077tail", false, 0, 77},
        {"077 ", false, 0, 77}, {" 077", false, 0, 77}, {"-077", false, 0, -63},
        {"-077tail", false, 0, -77}, {"0x100", true, 10, 100},
        {"0x100", true, 36, 1296}, {" 0x100", true, 36, 1540944},
        {"-0x100", false, 0, nan}, {"-0x100", true, 36, 1540944},
        {" -0x100", true, 36, -1540944}, {"+0x123", true, 33, nan},
        {"+0x123", true, 34, 1298259}, {"0x -100", false, 0, -256},
        {" 0x-100", false, 0, 0}, {"0x+0X100", false, 0, 0},
        {"100", true, 0, nan}, {"100", true, nan, nan},
        {"11", true, 4294967298.0, 3}, {"11", true, 3.8, 4},
        {"\t\n\r 42", false, 0, 42}, {"\v42", false, 0, nan},
        {"\f42", false, 0, nan}, {"\302\24042", false, 0, nan},
        {"", false, 0, nan}, {"0x", false, 0, nan},
        {"-0", false, 0, -0.0}, {"-0tail", false, 0, -0.0}
    };
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; FVM_Value out, args[2];
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        assert(fvm_call(c, "parseInt", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i)
            check_parse_int(c, cases[i].text, std::strlen(cases[i].text), cases[i].explicit_radix,
                            cases[i].radix, cases[i].expected);
        const char unterminated[] = {'4','2'};
        check_parse_int(c, unterminated, 2, false, 0, 42);
        check_parse_int(c, "077\0", 4, false, 0, 77);
        check_parse_int(c, "0x10\0more", 9, true, 10, 10);
        assert(fvm_value_string(args, "10") == FVM_OK); fvm_value_undefined(args + 1);
        assert(fvm_call(c, "parseInt", 2, args, &out) == FVM_OK && out.number != out.number);
        FVM_Object *text_object = 0, *radix_object = 0;
        assert(fvm_object_create(c, &text_object) == FVM_OK && fvm_gc_pin(c, text_object) == FVM_OK);
        assert(fvm_object_create(c, &radix_object) == FVM_OK && fvm_gc_pin(c, radix_object) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, text_object, "toString", parsing_text, 0, 0) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, radix_object, "valueOf", parsing_radix, 0, 0) == FVM_OK);
        fvm_value_undefined(args); args[0].type = FVM_VALUE_OBJECT; args[0].object = text_object;
        fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_OBJECT; args[1].object = radix_object;
        for (parsing_mode = 0; parsing_mode < 5; ++parsing_mode) {
            parsing_order = 0;
            FVM_Result status = fvm_call(c, "parseInt", 2, args, &out);
            if (parsing_mode == 2 || parsing_mode == 3) {
                assert(status == FVM_EXCEPTION);
                FVM_Value error; assert(fvm_exception_get(c, &error) == FVM_OK);
                assert(!std::strcmp(error.string, parsing_mode == 2 ? "radix failed" : "text failed"));
                assert(fvm_exception_clear(c) == FVM_OK);
            } else {
                assert(status == FVM_OK && out.type == FVM_VALUE_NUMBER);
                assert(parsing_mode == 0 ? out.number == 100 : out.number != out.number);
            }
            assert(parsing_order == (parsing_mode == 1 || parsing_mode == 2 ? 2U : 21U));
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth && !c->primitive_depth && !c->string_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
