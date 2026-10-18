/* Loaded initializer counts coerce, truncate and reject ranges without consuming values. */
#include "flashvm/avm1.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void text(Bytes &b, const char *s)
{
    size_t n = std::strlen(s) + 2;
    b.push_back(0x96); b.push_back((unsigned char)n); b.push_back((unsigned char)(n >> 8));
    b.push_back(0); do { b.push_back((unsigned char)*s); } while (*s++);
}
static void global(Bytes &b, const char *name) { text(b, name); b.push_back(0x1c); }
static FVM_Result run(FVM_Context *c, unsigned op, const FVM_Value &count, bool with_value)
{
    assert(fvm_set_global(c, "count", &count) == FVM_OK);
    Bytes code;
    text(code, "result");
    if (with_value) {
        if (op == 0x43) text(code, "key");
        text(code, "value");
    }
    global(code, "count"); code.push_back((unsigned char)op);
    code.push_back(0x1d); code.push_back(0);
    return fvm_avm1_execute(c, code.begin(), code.size());
}
static unsigned traces;
static FVM_Result trace(FVM_Context *, FVM_Object *, unsigned int n, const FVM_Value *args,
                         FVM_Value *out, void *)
{
    assert(n == 1 && args[0].type == FVM_VALUE_STRING);
    const char *expected[] = {"undefined", "c", "b", "a"};
    assert(traces < 4 && !std::strcmp(args[0].string, expected[traces++]));
    fvm_value_undefined(out); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_avm1_set_trace_rooted(c, trace, 0, 0) == FVM_OK);
    for (unsigned op = 0x42; op <= 0x43; ++op) {
        FVM_Value count, out;
        fvm_value_number(&count, 4294967296.0);
        assert(fvm_set_global(c, "count", &count) == FVM_OK);
        Bytes code; text(code, "a"); text(code, "b"); text(code, "c"); global(code, "count");
        code.push_back((unsigned char)op);
        for (unsigned i = 0; i < 4; ++i) code.push_back(0x26);
        code.push_back(0); traces = 0;
        assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK && traces == 4);
        fvm_value_number(&count, -0.5);
        assert(run(c, op, count, false) == FVM_OK);
        assert(fvm_get_global(c, "result", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        fvm_value_string(&count, "1.9");
        assert(run(c, op, count, true) == FVM_OK);
        assert(fvm_get_global(c, "result", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
        FVM_Value value;
        if (op == 0x42) assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK);
        else assert(fvm_object_get(c, out.object, "key", &value) == FVM_OK);
        assert(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "value"));
        fvm_value_undefined(&count);
        assert(run(c, op, count, false) == FVM_OK);
        assert(fvm_get_global(c, "result", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
        fvm_value_number(&count, 65537);
        assert(run(c, op, count, false) == FVM_UNSUPPORTED);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
