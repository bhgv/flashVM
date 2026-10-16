/* call/apply use current boxing globals and fall back to the global receiver. */
#include "primitive_boxing_fixture.hpp"
#include <cstdio>

int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned kind = 0; kind < 14; ++kind)
    {
        if (version && version < 4) continue;
        BoxingFixture f;
        boxing_init(runtime, f, version, type, kind);
        FVM_Context *c = f.context;
        Program body;
        body.variable("this"); body.text("argSeen"); body.variable("arg"); body.op(0x4f);
        body.variable("this"); body.finish();
        FVM_Object *function = 0, *array = 0;
        const char *parameters[] = {"arg"};
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 1, parameters, 0, &function) == FVM_OK);
        FVM_Value value = class_value(function), out, args[2], call_method, apply_method;
        assert(fvm_set_global(c, "boxingTarget", &value) == FVM_OK &&
               fvm_object_get(c, function, "call", &call_method) == FVM_OK &&
               fvm_object_get(c, function, "apply", &apply_method) == FVM_OK &&
               fvm_gc_pin(c, call_method.object) == FVM_OK && fvm_gc_pin(c, apply_method.object) == FVM_OK);
        assert((version ? fvm_object_create(c, &array) : fvm_construct(c, "Array", 0, 0, &array)) == FVM_OK);
        fvm_value_number(&value, 1);
        assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        fvm_value_number(&value, 81);
        assert((version ? fvm_object_set(c, array, "0", &value) : fvm_table_set_index(c, array, 0, &value)) == FVM_OK);
        value = class_value(array);
        assert(fvm_set_global(c, "boxingArguments", &value) == FVM_OK);
        for (unsigned apply = 0; apply < 2; ++apply)
        for (unsigned route = 0; route < 5; ++route)
        {
            unsigned mode = route < 2 ? 0 : route - 2;
            if (mode && !fvm_jit_available()) continue;
            FVM_Result expected = boxing_expected(f);
            f.reads = f.collections = f.blocked_collections = 0;
            args[0] = f.input;
            if (apply) args[1] = class_value(array); else fvm_value_number(args + 1, 81);
            if (route < 2)
            {
                FVM_Result status = route ? fvm_function_invoke(c, apply ? apply_method.object : call_method.object,
                                                               function, 2, args, &out)
                                          : fvm_object_call(c, function, apply ? "apply" : "call", 2, args, &out);
                assert(status == expected);
                if (status == FVM_EXCEPTION)
                    assert(c->exception_pending && c->exception_value.object == f.failure && fvm_exception_clear(c) == FVM_OK);
            }
            else
            {
                Program code;
                if (apply) code.variable("boxingArguments"); else code.number(81);
                code.variable("boxingInput"); code.number(2);
                code.variable("boxingTarget"); code.text(apply ? "apply" : "call"); code.op(0x52); code.finish();
                out = boxing_run(c, code, mode, expected);
            }
            if (expected == FVM_OK)
            {
                assert(out.type == FVM_VALUE_OBJECT && fvm_gc_pin(c, out.object) == FVM_OK);
                boxing_check(f, out.object, true);
                assert(fvm_object_get(c, out.object, "argSeen", &value) == FVM_OK);
                if (value.type != FVM_VALUE_NUMBER || value.number != 81)
                    std::fprintf(stderr, "argSeen v%u type%u kind%u apply%u route%u: type=%u number=%g\n", version,type,kind,apply,route,(unsigned)value.type,value.type==FVM_VALUE_NUMBER?value.number:0);
                assert(value.type == FVM_VALUE_NUMBER && value.number == 81);
            }
            unsigned reads = version && kind >= 6 && kind <= 9 ? 1U : 0U;
            assert(f.reads == reads && f.collections == reads && !f.blocked_collections);
            assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending && fvm_gc_collect(c, 0) == FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
