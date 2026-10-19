/* Inherited AVM1 input methods retain source identity through graph transfer, GC and actual JIT. */
#include "clip_input_methods_fixture.hpp"
/* Run script handlers after destroying the original graph owner. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version)
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            if (mode && !fvm_jit_available()) continue;
            FVM_Context *source = clip_context(runtime), *dest = clip_context(runtime);
            FVM_Object *root = method_root(source, version), *proto = 0, *function = 0;
            FVM_Object *actor = get(source, root, "actor").object;
            assert(fvm_object_create(source, &proto) == FVM_OK);
            assert(fvm_object_set_prototype(source, actor, proto) == FVM_OK);
            Bytes body = action(84);
            assert(fvm_function_create_avm1(source, body.begin(), body.size(), 0, 0, 0, &function) == FVM_OK);
            FVM_Value value = method_value(function);
            assert(fvm_object_set(source, proto, "onMouseMove", &value) == FVM_OK);
            value = method_value(root);
            unsigned char *wire = 0;
            size_t size = 0;
            assert(fvm_value_serialize(source, &value, &wire, &size) == FVM_OK);
            fvm_context_destroy(source);
            assert(fvm_value_deserialize(dest, wire, size, &value) == FVM_OK);
            fvm_serialized_free(wire);
            root = value.object;
            assert(fvm_gc_pin(dest, root) == FVM_OK);
            assert(fvm_gc_string_mode(dest, FVM_GC_STRINGS_TRACED) == FVM_OK);
            assert(fvm_gc_collect(dest, 0) == FVM_OK);
            assert(fvm_execution_set_mode(dest, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK);
            unsigned long before = dest->execution_mode.stats.native_entries;
            assert(fvm_movieclip_dispatch_input(dest, root, 8) == FVM_OK);
            assert((dest->execution_mode.stats.native_entries > before) == (mode != 0));
            number(dest, get(dest, root, "actor").object, "seen", 84);
            input_clean(dest);
            fvm_context_destroy(dest);
        }
    fvm_runtime_destroy(runtime);
}
