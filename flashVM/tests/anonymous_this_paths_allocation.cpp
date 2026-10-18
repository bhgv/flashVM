/* Qualified Undefined this paths preserve metadata and traced strings across OOM/retry. */
#define main anonymous_super_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_THIS_PATH_FIXTURE_ONLY
#include "anonymous_this_paths.cpp"
#include "../src/core/call_scope_constants.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1024 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_object_set_prototype(c, c->global, 0) == FVM_OK);
            FVM_Value link = class_value(c->global), marker; assert(fvm_set_global(c, "child", &link) == FVM_OK); fvm_value_number(&marker, 81); assert(fvm_set_global(c, "marker", &marker) == FVM_OK);
            FVM_Object *function = this_path_function(c, "this:child:marker", 0); assert(fvm_gc_pin(c, function) == FVM_OK);
            FVM_Value owner = class_value(function), name, value, out; fvm_value_string(&name, ""); fvm_value_string_bytes(&value, "r\0s", 3); allowance = boundary;
            FVM_Result status = fvm_exec_call_method(c, 0, &owner, &name, c->global, 1, &value, &out); allowance = -1;
            assert(!c->calling_this_undefined && !c->script_this_undefined && !c->script_this && !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_exec_call_method(c, 0, &owner, &name, c->global, 1, &value, &out) == FVM_OK); }
            else { assert(status == FVM_OK); success = true; }
            assert(out.type == FVM_VALUE_UNDEFINED);
            assert(fvm_get_global(c, "marker", &marker) == FVM_OK && marker.type == FVM_VALUE_NUMBER && marker.number == 81);
            assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
