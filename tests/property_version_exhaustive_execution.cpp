/* Reused interpreted/explicit/automatic native bodies observe every UInt16 flag combination. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned mode = 0; mode < 3; ++mode) {
        if (mode && !fvm_jit_available()) continue;
        FVM_Context *c = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        version_load(c, version);
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *child = 0, *parent = 0;
        assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        assert(fvm_object_set_prototype(c, child, parent) == FVM_OK);
        FVM_Value value, out;
        fvm_value_number(&value, 11);
        assert(fvm_object_set(c, parent, "field", &value) == FVM_OK);
        fvm_value_number(&value, 22);
        assert(fvm_object_set(c, child, "field", &value) == FVM_OK);
        value = class_value(child);
        assert(fvm_set_global(c, "exhaustiveTarget", &value) == FVM_OK);
        Program body; body.variable("exhaustiveTarget"); body.text("field"); body.op(0x4e); body.finish();
        FVM_Object *function = 0; FVM_JitCode *code = 0;
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &function) == FVM_OK);
        assert(fvm_gc_pin(c, function) == FVM_OK);
        if (mode == 1) {
            assert(fvm_jit_compile(body.bytes.begin(), body.bytes.size(), &code) == FVM_OK);
            assert(fvm_function_set_body(c, function, fvm_jit_body, code, 0) == FVM_OK);
        }
        assert(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
        for (unsigned flags = 0; flags <= 65535; ++flags) {
            version_flags(c, child, "field", flags, 65535);
            const unsigned long before = c->execution_mode.stats.native_entries;
            assert(fvm_function_invoke(c, function, 0, 0, 0, &out) == FVM_OK);
            assert(out.type == FVM_VALUE_NUMBER && out.number == ((flags & version_masks[version - 4]) ? 11 : 22));
            assert(c->execution_mode.stats.native_entries == before + (mode != 0));
            assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->calling_owner);
            if (!(flags & 4095)) assert(fvm_gc_collect(c, 0) == FVM_OK);
        }
        assert(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK);
        if (code) {
            assert(!code->active && fvm_function_set_body(c, function, 0, 0, 0) == FVM_OK);
            assert(fvm_jit_destroy(code) == FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
