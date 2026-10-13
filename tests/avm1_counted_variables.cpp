/* Counted variable names stay distinct through global/native and lexical execution. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"
static const char full[] = {'n', 0, 'x'};
static int native_calls;
static FVM_Result native(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *raw)
{
    ++native_calls;
    return fvm_object_get(c, (FVM_Object *)raw, "marker", out);
}
static int constructor_calls;
static FVM_Result constructor(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                               FVM_Value *out, void *raw)
{
    ++constructor_calls;
    fvm_value_undefined(out);out->type = FVM_VALUE_OBJECT;out->object = (FVM_Object *)raw;
    return FVM_OK;
}
static FVM_Result thrower(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                          FVM_Value *, void *)
{
    throw 42;
}
static void execute(FVM_Context *c, Bytes code)
{
    code.push_back(0);
    assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
}
static void named_body(Bytes &code, const char *name, const Bytes &body)
{
    Bytes record;
    do { record.push_back((unsigned char)*name); } while (*name++);
    record.push_back(0);record.push_back(0);
    record.push_back((unsigned char)body.size());record.push_back((unsigned char)(body.size() >> 8));
    for (size_t i = 0; i < body.size(); ++i)record.push_back(body[i]);
    code.push_back(0x9b);code.push_back((unsigned char)record.size());
    code.push_back((unsigned char)(record.size() >> 8));
    for (size_t i = 0; i < record.size(); ++i)code.push_back(record[i]);
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Value value, out;
    FVM_Object *capture;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_value_string_bytes(&value, full, sizeof(full)) == FVM_OK);
    assert(fvm_set_global(c, "key", &value) == FVM_OK);
    fvm_value_number(&value, 7);assert(fvm_set_global(c, "n", &value) == FVM_OK);
    {
        Bytes code;global(code, "key");integer(code, 42);code.push_back(0x1d);execute(c, code);
        assert(fvm_get_global_bytes(c, full, sizeof(full), &out) == FVM_OK && out.number == 42);
        assert(fvm_get_global(c, "n", &out) == FVM_OK && out.number == 7);
        Bytes body;global(body, "key");body.push_back(0x1c);out = run(c, body);
        assert(out.number == 42);
    }
    {
        /* Named functions retain declared and assigned counted lexical cells. */
        Bytes body, code;
        global(body, "key");integer(body, 55);body.push_back(0x3c);
        global(body, "key");integer(body, 66);body.push_back(0x1d);
        global(body, "key");body.push_back(0x1c);body.push_back(0x3e);body.push_back(0);
        named_body(code, "localTest", body);execute(c, code);
        assert(fvm_avm1_call(c, "localTest", 0, 0, 0, &out) == FVM_OK && out.number == 66);
        assert(fvm_get_global_bytes(c, full, sizeof(full), &out) == FVM_OK && out.number == 42);
        assert(fvm_get_global(c, "n", &out) == FVM_OK && out.number == 7);
    }
    {
        Bytes body, code;
        global(body, "key");body.push_back(0x41);
        global(body, "key");body.push_back(0x1c);body.push_back(0x3e);body.push_back(0);
        named_body(code, "declareTest", body);execute(c, code);
        assert(fvm_avm1_call(c, "declareTest", 0, 0, 0, &out) == FVM_OK);
        assert(out.type == FVM_VALUE_UNDEFINED);
    }
    assert(fvm_object_create(c, &capture) == FVM_OK);
    fvm_value_number(&value, 91);assert(fvm_object_set(c, capture, "marker", &value) == FVM_OK);
    assert(fvm_register_function_rooted_bytes(c, "f\0n", 3, native, capture, capture) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_call_bytes(c, "f\0n", 3, 0, 0, &out) == FVM_OK && out.number == 91);
    assert(fvm_get_global_bytes(c, "f\0n", 3, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT);
    assert(fvm_function_invoke(c, value.object, 0, 0, 0, &out) == FVM_OK && out.number == 91);
    assert(fvm_value_string_bytes(&value, "f\0n", 3) == FVM_OK);
    assert(fvm_set_global(c, "functionKey", &value) == FVM_OK);
    {
        Bytes body;integer(body, 0);global(body, "functionKey");body.push_back(0x3d);
        out = run(c, body);assert(out.number == 91);
    }
    {
        /* A missing full name must not invoke a live native prefix. */
        assert(fvm_register_function_rooted(c, "prefix", native, capture, capture) == FVM_OK);
        int before = native_calls;
        assert(fvm_call_bytes(c, "prefix\0suffix", 13, 0, 0, &out) == FVM_UNSUPPORTED);
        assert(native_calls == before);
        assert(fvm_value_string_bytes(&value, "prefix\0suffix", 13) == FVM_OK);
        assert(fvm_set_global(c, "functionKey", &value) == FVM_OK);
        Bytes code;integer(code, 0);global(code, "functionKey");code.push_back(0x3d);code.push_back(0);
        assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_UNSUPPORTED);
        assert(native_calls == before);
    }
    {
        const char pair[] = "\xed\xa0\x80\xed\xb0\x80";
        const char scalar[] = "\xf0\x90\x80\x80";
        assert(fvm_register_function_rooted(c, pair, native, capture, capture) == FVM_OK);
        assert(fvm_call(c, scalar, 0, 0, &out) == FVM_OK && out.number == 91);
        assert(fvm_get_global(c, pair, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    }
    {
        assert(fvm_register_constructor_rooted(c, "Widget", constructor, capture, capture) == FVM_OK);
        assert(fvm_value_string_bytes(&value, "Widget\0suffix", 13) == FVM_OK);
        assert(fvm_set_global(c, "constructorKey", &value) == FVM_OK);
        Bytes code;integer(code, 0);global(code, "constructorKey");code.push_back(0x40);code.push_back(0);
        assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_UNSUPPORTED);
        assert(constructor_calls == 0);
        assert(fvm_get_global(c, "Object", &value) == FVM_OK);
        assert(fvm_set_global_bytes(c, "Widget\0suffix", 13, &value) == FVM_OK);
        assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
        assert(constructor_calls == 0);
    }
    /* Explicit native calls retain precedence; script lookup honors an own undefined cell. */
    fvm_value_undefined(&value);
    assert(fvm_set_global_bytes(c, "f\0n", 3, &value) == FVM_OK);
    assert(fvm_get_global_bytes(c, "f\0n", 3, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_call_bytes(c, "f\0n", 3, 0, 0, &out) == FVM_OK && out.number == 91);
    assert(fvm_register_function_rooted_bytes(c, "t\0h", 3, thrower, 0, 0) == FVM_OK);
    assert(fvm_call_bytes(c, "t\0h", 3, 0, 0, &out) == FVM_ERROR);
    assert(out.type == FVM_VALUE_UNDEFINED);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
