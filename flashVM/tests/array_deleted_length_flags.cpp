/* Ordinary recreated numeric length preserves readonly flags while native tail deletion still runs. */
#define FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
#include "array_deleted_length.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"pop", "shift", "push", "unshift"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned method = 0; method < 4; ++method)
    for (unsigned readonly = 0; readonly < 2; ++readonly)
    for (unsigned protected_index = 0; protected_index < 2; ++protected_index)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value raw, out; unsigned count = 0; FVM_Object *array = array_deleted_fixture(c, 3, raw, count);
        if (readonly) assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
        if (protected_index) assert(fvm_object_set_property_flags(c, array, "0", 2, 0) == FVM_OK);
        out = array_deleted_call(c, array, methods[method], route);
        assert(out.type == FVM_VALUE_NUMBER && out.number == (method < 2 ? 11 : 2));
        assert(fvm_object_get(c, array, "length", &raw) == FVM_OK && raw.type == FVM_VALUE_NUMBER && raw.number == (readonly ? 1 : method < 2 ? 0 : 2));
        int own = 0; assert(fvm_object_has_own(c, array, "0", &own) == FVM_OK && !!own == (method >= 2 || protected_index));
        assert(fvm_table_get_index(c, array, 1, &raw) == FVM_OK && raw.number == (method == 2 ? 81 : method == 3 ? 11 : 22));
        unsigned flags = 0; assert(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK && flags == (readonly ? 4U : 0U));
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
