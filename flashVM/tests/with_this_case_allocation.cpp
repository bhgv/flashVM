/* SWF5 With keyword folding and counted-name rejection preserve outputs through OOM/retry. */
#define main with_case_allocator_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_WITH_THIS_CASE_FIXTURE_ONLY
#include "with_this_case.cpp"
#include "../src/core/this_cell.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned counted = 0; counted < 2; ++counted) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 256 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, 5); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *owner = 0, *scope = 0; assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK); FVM_Value value = class_value(owner), name, out; assert(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK && fvm_gc_pin(c, scope) == FVM_OK);
            fvm_value_string_bytes(&name, counted ? "THIS\0suffix" : "THIS", counted ? 11 : 4); fvm_value_number(&value, 91); assert(fvm_object_set_bytes(c, owner, name.string, fvm_string_size(name), &value) == FVM_OK);
            fvm_value_number(&value, 17);
            { FVM_ThisCellScope cell(c, value);
              FVM_Object *receiver = owner; int present = 19; fvm_value_number(&out, 77); allowance = boundary;
              FVM_Result status = fvm_exec_variable_lookup(c, scope, c->global, c->global, c->global, 1, &name, &out, &receiver, &present); allowance = -1;
              if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(out.number == 77 && receiver == owner && present == 19); assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_exec_variable_lookup(c, scope, c->global, c->global, c->global, 1, &name, &out, &receiver, &present) == FVM_OK); }
              else { assert(status == FVM_OK); success = true; }
              assert(out.type == FVM_VALUE_NUMBER && out.number == (counted ? 91 : 17) && present == 1 && receiver == (counted ? owner : c->global));
              assert(c->script_this_cell->value.number == 17 && !c->gc_frames);
            }
            assert(!c->script_this_cell && !c->script_this && !c->gc_native_depth); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
