/* Watch projects its primitive callback through current globals even when no Function results. */
#include "primitive_boxing_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned kind = 0; kind < 14; ++kind)
    {
        if (version && version < 4) continue;
        BoxingFixture f; boxing_init(rt, f, version, type, kind);
        FVM_Context *c = f.context;
        FVM_Object *target = 0; assert(fvm_construct(c, "Object", 0, 0, &target) == FVM_OK);
        FVM_Value value = class_value(target), method, args[2], out;
        assert(fvm_set_global(c, "watchTarget", &value) == FVM_OK && fvm_object_get(c, target, "watch", &method) == FVM_OK);
        assert(fvm_value_string_bytes(args, "watched\0x", 9) == FVM_OK); args[1] = f.input;
        assert(fvm_set_global(c, "watchName", args) == FVM_OK);
        for (unsigned route = 0; route < 5; ++route)
        {
            if (route >= 3 && !fvm_jit_available()) continue;
            f.reads = f.collections = f.blocked_collections = 0;
            FVM_Result expected = boxing_expected(f);
            if (!route) assert(fvm_object_call(c, target, "watch", 2, args, &out) == expected);
            else if (route == 1) assert(fvm_function_invoke(c, method.object, target, 2, args, &out) == expected);
            else {
                Program code; code.variable("boxingInput"); code.variable("watchName"); code.number(2); code.variable("watchTarget"); code.text("watch"); code.op(0x52); code.finish();
                out = boxing_run(c, code, route - 2, expected);
            }
            if (expected == FVM_OK) assert(out.type == FVM_VALUE_BOOLEAN && !out.boolean);
            else if (expected == FVM_EXCEPTION && route < 2) assert(c->exception_pending && c->exception_value.object == f.failure && fvm_exception_clear(c) == FVM_OK);
            unsigned reads = version && kind >= 6 && kind <= 9 ? 1U : 0U;
            assert(f.reads == reads && !f.collections && f.blocked_collections == reads && target->watchers.empty());
            assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending && fvm_gc_collect(c, 0) == FVM_OK);
        }
        for (unsigned count = 0; count < 4; ++count) {
            FVM_Value nullish[2]; nullish[0] = args[0]; fvm_value_undefined(nullish + 1); if (count == 3) nullish[1].type = FVM_VALUE_NULL;
            f.reads = 0;
            assert(fvm_object_call(c, target, "watch", count < 2 ? count : 2, nullish, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !out.boolean && !f.reads);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
