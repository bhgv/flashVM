/* General indexed writes use current own Array.length before flags and setter dispatch. */
#define FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
#include "array_deleted_length.cpp"
#include <cstdlib>
static FVM_Result index_live_write(FVM_Context *c, FVM_Object *array, const char *key, const FVM_Value &value, unsigned route, FVM_Result expected)
{
    if (!route) return fvm_object_set(c, array, key, &value);
    if (route == 4) return fvm_table_set_index(c, array, (unsigned long)std::strtoul(key, 0, 10), &value);
    FVM_Value target = class_value(array), out; assert(fvm_set_global(c, "indexLiveTarget", &target) == FVM_OK && fvm_set_global(c, "indexLiveValue", &value) == FVM_OK);
    Program p; p.variable("indexLiveTarget"); p.text(key); p.variable("indexLiveValue"); p.op(0x4f); p.number(1); p.finish(); out = boxing_run(c, p, route - 1, expected); (void)out; return expected;
}
#ifndef FVM_INDEX_LIVE_LENGTH_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"0", "1", "3", "\t+003", "-1", "2147483647"};
    const long indices[] = {0, 1, 3, 3, -1, 2147483647L};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind <= 10; ++kind)
    for (unsigned key = 0; key < 6; ++key)
    for (unsigned route = 0; route < 5; ++route)
    for (unsigned flags = 0; flags < 3; ++flags) {
        if ((route == 2 || route == 3) && !fvm_jit_available()) continue;
        if (route == 4 && (key == 3 || key == 4)) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value raw, input, out; unsigned count = 0; FVM_Object *array = array_deleted_fixture(c, kind, raw, count); assert(fvm_gc_pin(c, array) == FVM_OK);
        if (flags == 1 && kind != 1 && kind != 8 && kind != 9 && !(kind == 10 && version >= 7)) assert(fvm_object_set_property_flags(c, array, "length", FVM_PROPERTY_READ_ONLY, 0) == FVM_OK);
        if (flags == 2) assert(fvm_object_set_property_flags(c, array, keys[key], FVM_PROPERTY_READ_ONLY, 0) == FVM_OK);
        fvm_value_number(&input, 81); assert(index_live_write(c, array, keys[key], input, route, FVM_OK) == FVM_OK);
        const bool grown = indices[key] >= (long)count;
        const bool readonly_length = flags == 1 && kind != 1 && kind != 8 && kind != 9 && !(kind == 10 && version >= 7);
        if (grown && !readonly_length) {
            assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == (key == 5 ? -2147483648.0 : (double)indices[key] + 1));
        } else if (kind != 1 && kind != 8 && kind != 9 && !(kind == 10 && version >= 7)) { assert(fvm_object_get(c, array, "length", &out) == FVM_OK); boxing_equal(out, raw); }
        else { int own = 1; assert(fvm_object_has_own(c, array, "length", &own) == FVM_OK && !own); }
        if (flags != 2) assert(fvm_object_get(c, array, keys[key], &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 81);
        /* Growing from a Number through Int32 overflow truncates; raw String/Boolean tails survive. */
        if (key == 5 && kind <= 7) { assert(fvm_table_get_index(c, array, 1, &out) == FVM_OK); if (kind == 0) assert(out.type == FVM_VALUE_UNDEFINED); if (kind == 1 || kind == 2 || kind >= 5) assert(out.type == FVM_VALUE_NUMBER && out.number == 22); }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
