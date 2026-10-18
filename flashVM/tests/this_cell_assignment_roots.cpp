/* Active/suspended this strings and metadata survive child mutation, GC and errors. */
#define FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
#include "this_cell_assignment.cpp"
#include "../src/core/this_cell.hpp"
#include "flashvm/exception.h"
static FVM_Value expected_this;
static FVM_Object *this_capture;
static unsigned this_failure;
static FVM_Result this_probe(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    assert(c->script_this_cell && c->script_this_cell->previous);
    boxing_equal(c->script_this_cell->value, expected_this);
    FVM_ThisCell *parent = c->script_this_cell->previous;
    boxing_equal(parent->value, expected_this);
    int removed = 0;
    assert(fvm_object_delete(c, c->global, "replacement", &removed) == FVM_OK && removed);
    assert(fvm_object_delete(c, c->global, "probe", &removed) == FVM_OK && removed);
    assert(fvm_object_delete(c, c->global, "undefinedThisFunction2", &removed) == FVM_OK && removed);
    char bytes[] = {'a',0,'b'}; FVM_Value name, value;
    fvm_value_string(&name, "this"); fvm_value_string_bytes(&value, bytes, sizeof(bytes));
    assert(fvm_exec_variable_set(c, 0, c->global, c->global, &name, &value) == FVM_OK);
    bytes[0] = '?';
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    value = c->script_this_cell->value;
    assert(value.type == FVM_VALUE_STRING && fvm_string_size(value) == 3 && !std::memcmp(value.string, "a\0b", 3));
    boxing_equal(parent->value, expected_this); assert(parent->value.metatable == expected_this.metatable);
    bool capture_alive = false, edge_alive = expected_this.type != FVM_VALUE_OBJECT && !expected_this.metatable;
    FVM_Object *edge = expected_this.type == FVM_VALUE_OBJECT ? expected_this.object : expected_this.metatable;
    for (size_t i = 0; i < c->objects.size(); ++i) { capture_alive |= c->objects[i] == this_capture; edge_alive |= c->objects[i] == edge; }
    assert(capture_alive && edge_alive);
    if (this_failure == 1) { fvm_value_number(out, 77); return fvm_exception_raise(c, out); }
    if (this_failure == 2) return FVM_ERROR;
    fvm_value_number(out, 1); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 7; version <= 8; ++version)
    for (unsigned type = 0; type < 3; ++type)
    for (this_failure = 0; this_failure < 3; ++this_failure)
    for (unsigned mode = 0; mode < 3; ++mode) {
        if (mode && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version);
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *edge = 0; assert(fvm_construct(c, "Object", 0, 0, &edge) == FVM_OK);
        if (type == 0) expected_this = class_value(edge);
        if (type == 1) { fvm_value_string_bytes(&expected_this, "p\0q", 3); expected_this.metatable = edge; }
        if (type == 2) { fvm_value_number(&expected_this, 19); expected_this.metatable = edge; }
        assert(fvm_set_global(c, "replacement", &expected_this) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &this_capture) == FVM_OK);
        FVM_Object *probe = undefined_this_function(c, 5); assert(fvm_function_set_body(c, probe, this_probe, 0, this_capture) == FVM_OK);
        FVM_Value value = class_value(probe); assert(fvm_set_global(c, "probe", &value) == FVM_OK);
        Program body; body.text("this"); body.variable("replacement"); body.op(0x1d); body.number(0); body.text("probe"); body.op(0x3d); body.op(0x17); body.variable("this"); body.finish();
        FVM_Result status = this_failure == 1 ? FVM_EXCEPTION : this_failure == 2 ? FVM_ERROR : FVM_OK;
        value = boxing_run(c, body, mode, status);
        if (!this_failure) { boxing_equal(value, expected_this); assert(value.metatable == expected_this.metatable); }
        assert(!c->script_this_cell && !c->script_this && !c->script_this_undefined && !c->gc_frames && !c->script_depth && !c->exception_pending);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
