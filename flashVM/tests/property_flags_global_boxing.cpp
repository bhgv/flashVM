/* ASSetPropFlags boxes its target before numeric flags and property-list conversion. */
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
        FVM_Value args[4], function, out; args[0] = f.input; fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_NULL;
        fvm_value_number(args + 2, 1); fvm_value_number(args + 3, 0);
        assert(fvm_get_global(c, "ASSetPropFlags", &function) == FVM_OK);
        for (unsigned route = 0; route < 5; ++route) {
            if (route >= 3 && !fvm_jit_available()) continue;
            f.reads = f.collections = f.blocked_collections = 0;
            FVM_Result expected = version ? boxing_expected(f) : FVM_INVALID_ARGUMENT;
            if (!route) assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == expected);
            else if (route == 1) assert(fvm_function_invoke(c, function.object, 0, 4, args, &out) == expected);
            else {
                Program code; code.number(0); code.number(1); code.op(0x96); code.bytes.push_back(1); code.bytes.push_back(0); code.bytes.push_back(2);
                code.variable("boxingInput"); code.number(4); code.text("ASSetPropFlags"); code.op(0x3d); code.finish();
                out = boxing_run(c, code, route - 2, expected);
            }
            if (expected == FVM_OK) assert(out.type == FVM_VALUE_UNDEFINED);
            if (expected == FVM_EXCEPTION && route < 2) assert(c->exception_pending && c->exception_value.object == f.failure && fvm_exception_clear(c) == FVM_OK);
            unsigned reads = version && kind >= 6 && kind <= 9 ? 1U : 0U;
            assert(f.reads == reads && !f.collections && f.blocked_collections == reads);
            assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending && fvm_gc_collect(c, 0) == FVM_OK);
        }
        if (version) for (unsigned kind = 0; kind < 3; ++kind) {
            fvm_value_undefined(args); if (kind == 1) args[0].type = FVM_VALUE_NULL;
            assert(fvm_call(c, "ASSetPropFlags", kind == 2 ? 0 : 4, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
