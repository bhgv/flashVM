/* Collecting and throwing coercions preserve the player-specific string conversion order. */
#define FVM_STRING_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_strings.cpp"


struct StringPayload
{
    unsigned id;
    const char *text;
    double number;
};

static unsigned observed[32], observed_count, throw_id;


static FVM_Result string_conversion(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *raw)
{
    StringPayload *p = (StringPayload *)raw;
    if (!(observed_count < 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    observed[observed_count++] = p->id;
    FVM_Result status = fvm_gc_collect(c, 0);
    if (status != FVM_OK)
    {
        return status;
    }
    if (throw_id == p->id)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (p->text)
    {
        return fvm_value_string(out, p->text);
    }
    fvm_value_number(out, p->number);
    return FVM_OK;
}


static FVM_Value string_object(FVM_Context *c, StringPayload *payload)
{
    const unsigned char empty[] = {0};
    FVM_Object *owner = 0, *fn = 0;
    FVM_Value value;
    if (!(fvm_object_create(c, &owner) == FVM_OK && fvm_gc_pin(c, owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, string_conversion, payload, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = fn;
    if (!(fvm_object_set(c, owner, payload->text ? "toString" : "valueOf", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value.object = owner;
    return value;
}


static void conversion_order(const unsigned *expected, unsigned count)
{
    if (!(observed_count == count * 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < observed_count; ++i)
    {
        if (!(observed[i] == expected[i % count]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        StringPayload left = {1, "abcd", 0}, right = {2, "b", 0}, start = {2, 0, 2}, length = {3, 0, 2};
        FVM_Value args[3];
        args[0] = string_object(c, &left);
        args[1] = string_object(c, &right);
        const unsigned char binary[] = {0x21, 0x13, 0x29, 0x68};
        for (unsigned i = 0; i < 4; ++i)
        {
            Program p = string_program(binary[i], 2);
            observed_count = 0;
            throw_id = 0;
            string_check(c, p, 2, args);
            const unsigned order[] = {i == 3 ? 1U : 2U, i == 3 ? 2U : 1U};
            conversion_order(order, 2);
            observed_count = 0;
            throw_id = 2;
            FVM_Value unused;
            fvm_value_undefined(&unused);
            string_check(c, p, 2, args, &unused, FVM_EXCEPTION);
            if (i == 3)
            {
                conversion_order(order, 2);
            }
            else
            {
                const unsigned failed[] = {2};
                conversion_order(failed, 1);
            }
        }
        args[1] = string_object(c, &start);
        args[2] = string_object(c, &length);
        for (unsigned multibyte = 0; multibyte < 2; ++multibyte)
        {
            Program p = string_program(multibyte ? 0x35 : 0x15, 3);
            observed_count = 0;
            throw_id = 0;
            string_check(c, p, 3, args);
            const unsigned order[] = {3, 2, 1};
            conversion_order(order, 3);
            observed_count = 0;
            throw_id = 2;
            FVM_Value unused;
            fvm_value_undefined(&unused);
            string_check(c, p, 3, args, &unused, FVM_EXCEPTION);
            const unsigned failed[] = {3, 2};
            conversion_order(failed, 2);
        }
        const unsigned char unary[] = {0x14, 0x31, 0x32, 0x36, 0x33, 0x37};
        for (unsigned i = 0; i < 6; ++i)
        {
            FVM_Value arg = i < 4 ? args[0] : args[1];
            const unsigned order[] = {i < 4 ? 1U : 2U};
            observed_count = 0;
            throw_id = 0;
            string_check(c, string_program(unary[i], 1), 1, &arg);
            conversion_order(order, 1);
            observed_count = 0;
            throw_id = order[0];
            FVM_Value unused;
            fvm_value_undefined(&unused);
            string_check(c, string_program(unary[i], 1), 1, &arg, &unused, FVM_EXCEPTION);
            conversion_order(order, 1);
        }
        throw_id = 0;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
