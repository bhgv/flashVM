/* Root keyword and ordinary counted-prefix lookup preserve output atomicity across OOM. */
#define main root_case_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_PATH_ROOT_CASE_FIXTURE_ONLY
#include "path_root_case.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"_root", "_ROOT", "_Root", "_ROOT\0suffix"}; const size_t sizes[] = {5,5,5,12};
    for (unsigned version = 5; version <= 8; ++version)
    for (unsigned key = 0; key < 4; ++key) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 256 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *owner = 0, *shadow = 0, *scope = 0; assert(fvm_object_create(c, &owner) == FVM_OK && fvm_object_create(c, &shadow) == FVM_OK);
            FVM_Value value = class_value(shadow), path, out; assert(fvm_object_set_bytes(c, owner, keys[key], sizes[key], &value) == FVM_OK);
            value = class_value(owner); assert(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK && fvm_gc_pin(c, scope) == FVM_OK);
            assert(fvm_value_string_bytes(&path, keys[key], sizes[key]) == FVM_OK); fvm_value_number(&out, 77);
            allowance = boundary; const FVM_Result status = fvm_exec_scope_target(c, scope, c->global, c->global, &path, &out); allowance = -1;
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(out.type == FVM_VALUE_NUMBER && out.number == 77);
                assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_exec_scope_target(c, scope, c->global, c->global, &path, &out) == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            assert(out.type == FVM_VALUE_OBJECT && out.object == (!key ? c->global : shadow));
            assert(!c->gc_frames && !c->gc_native_depth && !c->script_this_cell); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
