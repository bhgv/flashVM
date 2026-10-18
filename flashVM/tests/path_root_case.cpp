/* Only exact first-token _root is a loaded path keyword; other spellings are ordinary properties. */
#include "property_version_fixture.hpp"
#include <cstdio>
static void root_case_word(Program &p, size_t n) { p.op((unsigned char)n); p.op((unsigned char)(n >> 8)); }
static void root_case_append(Program &p, const Program &b) { for (size_t i = 0; i < b.bytes.size(); ++i) p.op(b.bytes[i]); }
static void root_case_program(Program &p, const char *path, unsigned operation)
{
    Program body;
    if (operation == 1) { body.text(path); body.number(73); body.op(0x1d); body.variable(path); }
    else if (operation == 2) { body.number(0); body.text(path); body.op(0x3d); }
    else body.variable(path);
    body.op(0x3e); p.variable("owner"); p.op(0x94); root_case_word(p, 2); root_case_word(p, body.bytes.size()); root_case_append(p, body); p.number(99); p.finish();
}
static FVM_Object *root_case_method(FVM_Context *c)
{
    Program body; body.variable("this"); body.text("marker"); body.op(0x4e); body.finish(); FVM_Object *fn = 0;
    assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &fn) == FVM_OK); return fn;
}
static void root_case_global(FVM_Context *c, FVM_Object *owner)
{
    FVM_Value value = class_value(owner); assert(fvm_set_global(c, "owner", &value) == FVM_OK);
    fvm_value_number(&value, 17); assert(fvm_set_global(c, "marker", &value) == FVM_OK);
    value = class_value(c->global); assert(fvm_set_global(c, "child", &value) == FVM_OK);
    value = class_value(root_case_method(c)); assert(fvm_set_global(c, "method", &value) == FVM_OK);
}
#ifndef FVM_PATH_ROOT_CASE_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *names[] = {"_root", "_ROOT", "_Root", "_rootSuffix"};
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned name = 0; name < 4; ++name)
    for (unsigned kind = 0; kind < 4; ++kind)
    for (unsigned operation = 0; operation < 3; ++operation)
    for (unsigned route = 0; route < 4; ++route)
    for (unsigned transport = 0; transport < 2; ++transport) {
        if ((version && version < 4) || (kind == 1 && version == 4) || (!version && operation == 2) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0, *source = 0; assert(fvm_context_create(rt, &c) == FVM_OK); if (version) version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *owner = 0, *shadow = 0; assert(fvm_object_create(c, &owner) == FVM_OK && fvm_object_create(c, &shadow) == FVM_OK);
        FVM_Value value = class_value(shadow), out; assert(fvm_object_set(c, owner, names[name], &value) == FVM_OK && fvm_object_set(c, shadow, "child", &value) == FVM_OK);
        value = class_value(owner); assert(fvm_object_set(c, owner, "holder", &value) == FVM_OK);
        fvm_value_number(&value, 91); assert(fvm_object_set(c, shadow, "marker", &value) == FVM_OK);
        value = class_value(root_case_method(c)); assert(fvm_object_set(c, shadow, "method", &value) == FVM_OK);
        if (transport) {
            source = c; assert(fvm_context_create(rt, &c) == FVM_OK); if (version) version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            value = class_value(owner); unsigned char *wire = 0; size_t size = 0;
            assert(fvm_value_serialize(source, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(c, wire, size, &value) == FVM_OK); fvm_serialized_free(wire); owner = value.object;
            assert(fvm_object_get(c, owner, names[name], &value) == FVM_OK && value.type == FVM_VALUE_OBJECT); shadow = value.object;
        }
        root_case_global(c, owner);
        char path[96]; const char *member = operation == 2 ? "method" : "marker";
        if (kind == 0) std::sprintf(path, "%s:%s", names[name], member);
        else if (kind == 1) std::sprintf(path, "%s.%s", names[name], member);
        else if (kind == 2) std::sprintf(path, "holder/%s:%s", names[name], member);
        else std::sprintf(path, "%s/child:%s", names[name], member);
        if (!route) {
            FVM_Object *scope = 0; value = class_value(owner); assert(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK); fvm_value_string(&value, path);
            if (operation == 1) { FVM_Value input; fvm_value_number(&input, 73); assert(fvm_exec_variable_set(c, scope, c->global, c->global, &value, &input) == FVM_OK); }
            if (operation == 2) assert(fvm_exec_call_function(c, scope, c->global, c->global, c->global, 0, &value, 0, 0, &out) == FVM_OK);
            else { int handled = 0, present = 0; assert(fvm_exec_variable_path(c, scope, c->global, c->global, &value, &out, &handled, &present) == FVM_OK && handled && present); }
        } else { Program p; root_case_program(p, path, operation); out = boxing_run(c, p, route - 1, FVM_OK); }
        const bool keyword = (!version || version >= 5) && !name && kind != 2;
        if (out.type != FVM_VALUE_NUMBER || out.number != (operation == 1 ? 73 : keyword ? 17 : 91))
            std::fprintf(stderr, "version=%u name=%s kind=%u operation=%u route=%u transport=%u type=%u value=%g\n", version, names[name], kind, operation, route, transport, (unsigned)out.type, out.number);
        assert(out.type == FVM_VALUE_NUMBER && out.number == (operation == 1 ? 73 : keyword ? 17 : 91));
        assert(fvm_object_get(c, shadow, "marker", &out) == FVM_OK && out.number == (operation == 1 && !keyword ? 73 : 91));
        assert(fvm_get_global(c, "marker", &out) == FVM_OK && out.number == (operation == 1 && keyword ? 73 : 17));
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_this_cell); fvm_context_destroy(c); if (source) fvm_context_destroy(source);
    }
    fvm_runtime_destroy(rt);
}
#endif
