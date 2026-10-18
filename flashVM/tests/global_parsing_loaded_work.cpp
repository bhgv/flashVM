/* Numeric prefix scans share loaded instruction budgets across native callbacks. */
#define FVM_PARSE_INT_LOADED_FIXTURE_ONLY
#include "global_parse_int_loaded.cpp"
static FVM_Result nested_parse(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *,
                               FVM_Value *out, void *)
{
    assert(c->avm1_execution_depth == 1);
    FVM_Value input, value; fvm_value_string(&input, "12345");
    FVM_Result status = fvm_call(c, "parseFloat", 1, &input, &value);
    assert(c->avm1_execution_depth == 1);
    if (status != FVM_OK) return status;
    assert(value.number == 12345);
    return fvm_value_string(out, "42");
}
static void limited(FVM_Context *c, const char *name, const char *text,
                     unsigned long work, double expected)
{
    FVM_Value input, out; fvm_value_string(&input, text);
    c->runtime->config.instruction_limit = work - 1;
    assert(fvm_call(c, name, 1, &input, &out) == FVM_ERROR);
    assert(out.type == FVM_VALUE_UNDEFINED && c->avm1_steps == work - 1);
    assert(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth);
    c->runtime->config.instruction_limit = work;
    assert(fvm_call(c, name, 1, &input, &out) == FVM_OK && out.number == expected);
    assert(c->avm1_steps == work && !c->avm1_execution_depth);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; FVM_Value input, out;
        assert(fvm_context_create(rt, &c) == FVM_OK); parsing_load(c, version);
        limited(c, "parseInt", "123", 3, 123);
        limited(c, "parseInt", "077", 5, 63);
        limited(c, "parseInt", " 12", 3, 12);
        limited(c, "parseFloat", "12.5", 8, 12.5);
        limited(c, "parseFloat", "1e2", 5, 100);
        limited(c, "parseFloat", "   .", 5, 0);
        const char *names[] = {"parseInt", "parseFloat"};
        fvm_detail::vector<char> long_text(65538);
        std::memset(long_text.begin(), '1', 65537);
        for (unsigned i = 0; i < 2; ++i) {
            fvm_value_string(&input, long_text.begin()); rt->config.instruction_limit = 64;
            assert(fvm_call(c, names[i], 1, &input, &out) == FVM_ERROR && c->avm1_steps == 64);
            fvm_value_string(&input, "123");
            c->avm1_execution_depth = 1; c->avm1_steps = 7; rt->config.instruction_limit = 8;
            assert(fvm_call(c, names[i], 1, &input, &out) == FVM_ERROR && c->avm1_steps == 8);
            assert(c->avm1_execution_depth == 1);
            c->avm1_steps = std::numeric_limits<unsigned long>::max(); rt->config.instruction_limit = 0;
            assert(fvm_call(c, names[i], 1, &input, &out) == FVM_ERROR);
            assert(c->avm1_steps == std::numeric_limits<unsigned long>::max() && c->avm1_execution_depth == 1);
            c->avm1_execution_depth = 0;
            assert(fvm_call(c, names[i], 1, &input, &out) == FVM_OK && out.number == 123);
        }
        FVM_Object *obj = 0;
        assert(fvm_object_create(c, &obj) == FVM_OK && fvm_gc_pin(c, obj) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, obj, "toString", nested_parse, 0, 0) == FVM_OK);
        fvm_value_undefined(&input); input.type = FVM_VALUE_OBJECT; input.object = obj;
        rt->config.instruction_limit = 6;
        assert(fvm_call(c, "parseInt", 1, &input, &out) == FVM_ERROR && c->avm1_steps == 6);
        assert(!c->avm1_execution_depth && !c->primitive_depth && !c->string_depth);
        rt->config.instruction_limit = 12;
        assert(fvm_call(c, "parseInt", 1, &input, &out) == FVM_OK && out.number == 42 && c->avm1_steps == 12);
        rt->config.instruction_limit = 0;
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    FVM_Context *host = 0; FVM_Value input, out;
    assert(fvm_context_create(rt, &host) == FVM_OK && fvm_standard_install(host) == FVM_OK);
    fvm_value_string(&input, "123"); rt->config.instruction_limit = 1;
    assert(fvm_call(host, "parseInt", 1, &input, &out) == FVM_OK && out.number == 123);
    assert(fvm_call(host, "parseFloat", 1, &input, &out) == FVM_OK && out.number == 123);
    fvm_context_destroy(host); fvm_runtime_destroy(rt);
}
