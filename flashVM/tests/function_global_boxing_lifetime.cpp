/* Current-global boxing survives apply collection and transferred adapter captures. */
#include "primitive_boxing_fixture.hpp"

struct BoxingLifetime
{
    BoxingFixture *fixture;
    FVM_Object *receiver;
    unsigned failure, calls;
};

static FVM_Result boxing_collect(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *raw)
{
    BoxingLifetime *probe = (BoxingLifetime *)raw;
    ++probe->calls;
    probe->receiver = 0;
    for (size_t i = 0; i < c->objects.size(); ++i)
        if (c->objects[i]->properties.find("boxed") != c->objects[i]->properties.end())
        {
            assert(!probe->receiver);
            probe->receiver = c->objects[i];
        }
    assert(probe->receiver && !c->gc_native_depth);
    FVM_Value absent;
    fvm_value_undefined(&absent);
    assert(fvm_set_global(c, probe->fixture->name, &absent) == FVM_OK &&
           fvm_set_global(c, "boxingInput", &absent) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    bool live = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
        if (c->objects[i] == probe->receiver) live = true;
    assert(live);
    FVM_Value boxed;
    assert(fvm_object_get(c, probe->receiver, "boxed", &boxed) == FVM_OK);
    boxing_equal(boxed, probe->fixture->input);
    if (probe->failure == 1) return FVM_ERROR;
    if (probe->failure == 2)
    {
        *out = class_value(probe->fixture->failure);
        return fvm_exception_raise(c, out);
    }
    fvm_value_number(out, 81);
    return FVM_OK;
}

static void test_collection(FVM_Runtime *runtime)
{
    for (unsigned version = 6; version <= 8; version += 2)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned mode = 0; mode < 4; ++mode)
    for (unsigned failure = 0; failure < 3; ++failure)
    {
        if (mode > 1 && !fvm_jit_available()) continue;
        BoxingFixture f;
        boxing_init(runtime, f, version, type, 5);
        FVM_Context *c = f.context;
        BoxingLifetime probe = {&f, 0, failure, 0};
        Program body;
        body.text("boxingTargetCalled"); body.number(1); body.op(0x1d);
        body.variable("this"); body.finish();
        FVM_Object *target = 0, *arguments = 0, *getter = 0;
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &target) == FVM_OK);
        FVM_Value value = class_value(target), out, args[3], klass, prototype, add;
        assert(fvm_set_global(c, "boxingTarget", &value) == FVM_OK);
        fvm_value_number(&value, 0);
        assert(fvm_set_global(c, "boxingTargetCalled", &value) == FVM_OK);
        assert(fvm_object_create(c, &arguments) == FVM_OK);
        value = class_value(arguments);
        assert(fvm_set_global(c, "boxingArguments", &value) == FVM_OK);
        fvm_value_number(&value, 1);
        assert(fvm_object_set(c, arguments, "length", &value) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK &&
               fvm_function_set_body(c, getter, boxing_collect, &probe, f.factory) == FVM_OK);
        assert(fvm_get_global(c, "Object", &klass) == FVM_OK &&
               fvm_object_get(c, klass.object, "prototype", &prototype) == FVM_OK &&
               fvm_object_get(c, prototype.object, "addProperty", &add) == FVM_OK);
        fvm_value_string(args, "0"); args[1] = class_value(getter);
        fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
        assert(fvm_function_invoke(c, add.object, arguments, 3, args, &out) == FVM_OK && out.boolean);
        FVM_Result expected = failure == 0 ? FVM_OK : failure == 1 ? FVM_ERROR : FVM_EXCEPTION;
        if (!mode)
        {
            args[0] = f.input; args[1] = class_value(arguments);
            assert(fvm_object_call(c, target, "apply", 2, args, &out) == expected);
            if (expected == FVM_EXCEPTION)
                assert(c->exception_value.object == f.failure && fvm_exception_clear(c) == FVM_OK);
        }
        else
        {
            Program code;
            code.variable("boxingArguments"); code.variable("boxingInput"); code.number(2);
            code.variable("boxingTarget"); code.text("apply"); code.op(0x52); code.finish();
            out = boxing_run(c, code, mode - 1, expected);
        }
        assert(probe.calls == 1 && probe.receiver);
        if (expected == FVM_OK)
        {
            assert(out.type == FVM_VALUE_OBJECT && out.object == probe.receiver && fvm_gc_pin(c, out.object) == FVM_OK);
            boxing_check(f, out.object, true);
        }
        assert(fvm_get_global(c, "boxingTargetCalled", &value) == FVM_OK &&
               value.type == FVM_VALUE_NUMBER && value.number == (failure ? 0 : 1));
        assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending && fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
}

static void test_transfer(FVM_Runtime *runtime)
{
    const char *names[] = {"Number", "Boolean", "String"};
    FVM_Context *source = 0;
    assert(fvm_context_create(runtime, &source) == FVM_OK && fvm_standard_install(source) == FVM_OK);
    FVM_Value klass, prototype;
    assert(fvm_get_global(source, "Function", &klass) == FVM_OK &&
           fvm_object_get(source, klass.object, "prototype", &prototype) == FVM_OK);
    for (unsigned version = 6; version <= 8; version += 2)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned apply = 0; apply < 2; ++apply)
    for (unsigned custom = 0; custom < 3; ++custom)
    {
        FVM_Value method, restored;
        assert(fvm_object_get(source, prototype.object, apply ? "apply" : "call", &method) == FVM_OK);
        unsigned char *data = 0;
        size_t size = 0;
        assert(fvm_value_serialize(source, &method, &data, &size) == FVM_OK);
        FVM_Context *c = 0;
        assert(fvm_context_create(runtime, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        /* Intentionally omit standard installation: captured donor constructors exist only in the wire graph. */
        BoxingFixture f;
        std::memset(&f, 0, sizeof(f));
        f.context = c; f.version = version; f.type = type; f.name = names[type];
        f.kind = custom == 2 ? 5 : 1;
        if (type == 0) fvm_value_number(&f.input, 42);
        if (type == 1) fvm_value_boolean(&f.input, 0);
        if (type == 2) fvm_value_string_bytes(&f.input, "a\0b", 3);
        if (custom == 2)
        {
            Program factory;
            factory.variable("this"); factory.text("boxed"); factory.variable("x"); factory.op(0x4f);
            factory.variable("this"); factory.finish();
            const char *params[] = {"x"};
            assert(fvm_function_create_avm1(c, factory.bytes.begin(), factory.bytes.size(), 1, params, 0, &f.factory) == FVM_OK);
            restored = class_value(f.factory);
            assert(fvm_set_global(c, f.name, &restored) == FVM_OK);
        }
        else if (custom == 1)
        {
            fvm_value_undefined(&restored);
            assert(fvm_set_global(c, f.name, &restored) == FVM_OK);
        }
        assert(fvm_value_deserialize(c, data, size, &restored) == FVM_OK && fvm_gc_pin(c, restored.object) == FVM_OK);
        fvm_serialized_free(data);
        FVM_Object *adapter = restored.object;
        FVM_Value binding;
        assert(fvm_get_global(c, f.name, &binding) == FVM_OK &&
               (custom == 2 ? binding.type == FVM_VALUE_OBJECT && binding.object == f.factory
                            : binding.type == FVM_VALUE_UNDEFINED));
        Program target;
        target.variable("this"); target.text("argSeen"); target.variable("arg"); target.op(0x4f);
        target.variable("this"); target.finish();
        const char *params[] = {"arg"};
        FVM_Object *function = 0, *arguments = 0;
        assert(fvm_function_create_avm1(c, target.bytes.begin(), target.bytes.size(), 1, params, 0, &function) == FVM_OK &&
               fvm_object_set(c, function, "invoke", &restored) == FVM_OK);
        FVM_Value value = class_value(function);
        assert(fvm_set_global(c, "boxingTarget", &value) == FVM_OK &&
               fvm_set_global(c, "boxingInput", &f.input) == FVM_OK && fvm_object_create(c, &arguments) == FVM_OK);
        fvm_value_number(&value, 1);
        assert(fvm_object_set(c, arguments, "length", &value) == FVM_OK);
        fvm_value_number(&value, 81);
        assert(fvm_object_set(c, arguments, "0", &value) == FVM_OK);
        value = class_value(arguments);
        assert(fvm_set_global(c, "boxingArguments", &value) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        for (unsigned mode = 0; mode < 4; ++mode)
        {
            if (mode > 1 && !fvm_jit_available()) continue;
            FVM_Value out;
            if (!mode)
            {
                FVM_Value args[2]; args[0] = f.input;
                if (apply) args[1] = class_value(arguments); else fvm_value_number(args + 1, 81);
                assert(fvm_function_invoke(c, adapter, function, 2, args, &out) == FVM_OK);
            }
            else
            {
                Program code;
                if (apply) code.variable("boxingArguments"); else code.number(81);
                code.variable("boxingInput"); code.number(2);
                code.variable("boxingTarget"); code.text("invoke"); code.op(0x52); code.finish();
                out = boxing_run(c, code, mode - 1, FVM_OK);
            }
            assert(out.type == FVM_VALUE_OBJECT && fvm_gc_pin(c, out.object) == FVM_OK);
            boxing_check(f, out.object, true);
            assert(fvm_object_get(c, out.object, "argSeen", &value) == FVM_OK && value.number == 81);
        }
        assert(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_context_destroy(source);
}

int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    test_collection(runtime);
    test_transfer(runtime);
    fvm_runtime_destroy(runtime);
    return 0;
}
