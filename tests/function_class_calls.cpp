/* Function calls preserve the raw first argument; empty loaded construction is ordinary. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/serialization.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value klass, proto, input, out;
        assert(fvm_get_global(c, "Function", &klass) == FVM_OK && fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK);
        for (unsigned kind = 0; kind < 8; ++kind)
        {
            fvm_value_undefined(&input);
            if (kind == 2) input.type = FVM_VALUE_NULL;
            if (kind == 3) fvm_value_number(&input, 42);
            if (kind == 4) fvm_value_string(&input, "a");
            if (kind == 5) input = proto;
            if (kind == 6) fvm_value_boolean(&input, 1);
            if (kind == 7) assert(fvm_value_string_bytes(&input, "a\0b", 3) == FVM_OK);
            unsigned count = kind ? 1 : 0;
            assert(fvm_set_global(c, "input", &input) == FVM_OK);
            for (unsigned route = 0; route < (version ? 11U : 5U); ++route)
            {
                if (route == 0) assert(fvm_call(c, "Function", count, count ? &input : 0, &out) == FVM_OK);
                else if (route == 1) assert(fvm_function_invoke(c, klass.object, 0, count, count ? &input : 0, &out) == FVM_OK);
                else {
                    if (!fvm_jit_available()) continue;
                    class_mode = route < 5 ? route - 2 : (route - 5) % 3;
                    Program code; if (count) code.variable("input"); code.number(count);
                    if (route >= 8) { code.variable("Function"); code.text(""); code.op(0x53); }
                    else { code.text("Function"); code.op(route >= 5 ? 0x40 : 0x3d); }
                    code.finish(); out = class_run(c, code);
                }
                if (!version) assert(out.type == FVM_VALUE_OBJECT && out.object->invocable.callback);
                else if (!count) assert(out.type == FVM_VALUE_OBJECT && out.object->prototype == (route >= 5 ? proto.object : 0) && !out.object->invocable.callback);
                else { assert(out.type == input.type); if (kind == 3) assert(out.number == 42); if (kind == 4) assert(fvm_string_size(out) == 1 && out.string[0] == 'a'); if (kind == 5) assert(out.object == proto.object); if (kind == 6) assert(out.boolean); if (kind == 7) assert(fvm_string_size(out) == 3 && !std::memcmp(out.string, "a\0b", 3)); }
            }
        }
        if (version) {
            FVM_Object *rejected = (FVM_Object *)1;
            fvm_value_number(&input, 42);
            assert(fvm_function_construct(c, klass.object, 1, &input, &rejected) == FVM_ERROR && !rejected);
        }
        FVM_Object *instance = 0;
        assert(fvm_function_construct(c, klass.object, 0, 0, &instance) == FVM_OK && instance->prototype == proto.object);
        assert(version ? !instance->invocable.callback : !!instance->invocable.callback);
        if (version) { FVM_Value identity; assert(fvm_object_get(c, instance, "__constructor__", &identity) == FVM_OK && identity.object == klass.object); assert(fvm_function_construct(c, klass.object, 1, &proto, &instance) == FVM_OK && instance == proto.object); }
        if (version && fvm_jit_available())
        for (unsigned mode = 0; mode < 3; ++mode)
        for (unsigned argument = 0; argument < 2; ++argument)
        {
            assert(fvm_set_global(c, "input", &proto) == FVM_OK);
            class_mode = mode;
            Program code;
            if (argument) code.variable("input");
            code.number(argument); code.text("Function"); code.op(0x40); code.finish();
            out = class_run(c, code);
            assert(out.type == FVM_VALUE_OBJECT);
            if (argument) assert(out.object == proto.object);
            else assert(!out.object->invocable.callback && out.object->prototype == proto.object);
        }
        FVM_Context *other = 0;
        assert(fvm_context_create(rt, &other) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); }
        unsigned char *data = 0; size_t size = 0;
        assert(fvm_value_serialize(c, &klass, &data, &size) == FVM_OK && fvm_value_deserialize(other, data, size, &out) == FVM_OK);
        fvm_serialized_free(data);
        FVM_Object *copy = out.object;
        assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        fvm_value_number(&input, 29);
        assert(fvm_function_invoke(other, copy, 0, 1, &input, &out) == FVM_OK);
        assert(version ? out.type == FVM_VALUE_NUMBER && out.number == 29 : out.type == FVM_VALUE_OBJECT);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
