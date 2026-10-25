/* Native literal count/key callbacks collect and throw; Array literals bypass prototype setters. */
#define FVM_LITERAL_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_literals.cpp"

static unsigned literal_calls, accessor_calls;
static bool literal_throw;


static FVM_Result literal_conversion(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *raw)
{
    ++literal_calls;
    FVM_Result status = fvm_gc_collect(c, 0);
    if (status != FVM_OK)
    {
        return status;
    }
    if (raw)
    {
        fvm_value_number(out, 1);
    }
    else
    {
        fvm_value_string(out, "key");
    }
    return literal_throw ? fvm_exception_raise(c, out) : FVM_OK;
}


static FVM_Value coercible(FVM_Context *c, bool numeric)
{
    const unsigned char empty[] = {0};
    FVM_Object *object = 0, *fn = 0;
    FVM_Value value;
    if (!(fvm_object_create(c, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, literal_conversion, numeric ? (void *)c : 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = fn;
    if (!(fvm_object_set(c, object, numeric ? "valueOf" : "toString", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value.object = object;
    return value;
}


static FVM_Result inherited(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++accessor_calls;
    fvm_value_undefined(out);
    return FVM_OK;
}
#ifndef FVM_LITERAL_CALLBACK_FIXTURE_ONLY


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value arg = coercible(c, false);
        Program object;
        object.variable("input");
        object.number(42);
        object.number(1);
        object.op(0x43);
        object.finish();
        const LiteralProperty property[] = {{"key", 3, FVM_VALUE_NUMBER, 42, 1}};
        literal_calls = 0;
        literal_throw = false;
        literal_check(c, object, &arg, FVM_OK, FVM_VALUE_OBJECT, property, 1);
        if (!(literal_calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        literal_calls = 0;
        literal_throw = true;
        literal_check(c, object, &arg, FVM_EXCEPTION, FVM_VALUE_UNDEFINED);
        if (!(literal_calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        literal_throw = false;
        arg = coercible(c, true);
        for (unsigned is_object = 0; is_object < 2; ++is_object)
        {
            Program p;
            if (is_object)
            {
                p.text("key");
            }
            p.number(42);
            p.variable("input");
            p.op(is_object ? 0x43 : 0x42);
            p.finish();
            const LiteralProperty value[] = {{is_object ? "key" : "0", is_object ? 3U : 1U, FVM_VALUE_NUMBER, 42, 1}};
            literal_calls = 0;
            literal_check(c, p, &arg, version ? FVM_OK : FVM_ERROR, FVM_VALUE_OBJECT, value, 1);
            if (!(literal_calls == (version ? 3U : 0U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                literal_calls = 0;
                literal_throw = true;
                literal_check(c, p, &arg, FVM_EXCEPTION, FVM_VALUE_UNDEFINED);
                if (!(literal_calls == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                literal_throw = false;
            }
        }
        FVM_Value array, proto;
        if (!(fvm_get_global(c, "Array", &array) == FVM_OK &&
              fvm_object_get(c, array.object, "prototype", &proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, proto.object, "0", inherited, inherited, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, proto.object, "1", inherited, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, proto.object, "1", 4, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program own;
        own.number(17);
        own.number(42);
        own.number(2);
        own.op(0x42);
        own.finish();
        const LiteralProperty properties[] = {{"0", 1, FVM_VALUE_NUMBER, 42, 1}, {"1", 1, FVM_VALUE_NUMBER, 17, 1}};
        accessor_calls = 0;
        literal_check(c, own, 0, FVM_OK, FVM_VALUE_OBJECT, properties, 2);
        if (!(!accessor_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

#endif
