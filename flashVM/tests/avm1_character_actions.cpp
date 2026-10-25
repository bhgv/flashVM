/* Character actions: UTF-16, wrapping, empty/NUL input, coercion errors and old-version gates. */
#define main legacy_string_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_legacy_string_actions.cpp"

#undef main


static FVM_Result fail_character(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    return FVM_OUT_OF_MEMORY;
}


static void unit(FVM_Context *c, unsigned op, double number, unsigned short expected)
{
    FVM_Value in;
    fvm_value_number(&in, number);
    FVM_Value out = unary(c, op, in);
    unsigned short got = 0;
    size_t length = 999;
    if (!(fvm_value_get_utf16(&out, &got, 1, &length) == FVM_OK && length == (expected ? 1U : 0U)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected)
    {
        if (!(got == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 5; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value in;
        for (unsigned op = 0x32; op <= 0x36; op += 4)
        {
            fvm_value_string_bytes(&in, "\0A", 2);
            if (!(unary(c, op, in).number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&in, "");
            if (!(unary(c, op, in).number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&in, "Abc");
            if (!(unary(c, op, in).number == 65))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&in, 123);
            if (!(unary(c, op, in).number == 49))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned short first = 0xd83d;
            if (!(fvm_value_string_utf16(c, &first, 1, &in) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_set_global(c, "input", &in) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes code;
            global(code, "input");
            code.push_back((unsigned char)op);
            code.push_back(0);
            if (!(fvm_avm1_execute(c, code.begin(), code.size()) == (version < 6 ? FVM_UNSUPPORTED : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version >= 6)
            {
                if (!(unary(c, op, in).number == 65533))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (version >= 6)
            {
                fvm_value_string(&in, "\xf0\x9f\x98\x80");
                if (!(unary(c, op, in).number == 65533))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string(&in, "\xc3\xa9");
                if (!(unary(c, op, in).number == 233))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        for (unsigned op = 0x33; op <= 0x37; op += 4)
        {
            unit(c, op, 0, 0);
            unit(c, op, 65.9, 65);
            unit(c, op, 65536, 0);
            unit(c, op, 65537, 1);
            unit(c, op, std::numeric_limits<double>::infinity(), 0);
            if (version >= 6)
            {
                unit(c, op, -1, 65535);
                unit(c, op, 0xd800, 65533);
                unit(c, op, 0xdfff, 65533);
                unit(c, op, 0x20ac, 0x20ac);
            }
        }
        FVM_Object *obj = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &obj) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, obj, "toString", fail_character, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, obj, "valueOf", fail_character, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&in);
        in.type = FVM_VALUE_OBJECT;
        in.object = obj;
        if (!(fvm_set_global(c, "input", &in) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned ops[] = {0x32, 0x33, 0x36, 0x37};
        for (unsigned i = 0; i < 4; ++i)
        {
            Bytes code;
            global(code, "input");
            code.push_back((unsigned char)ops[i]);
            code.push_back(0);
            if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char empty[] = {(unsigned char)ops[i], 0};
            if (!(fvm_avm1_execute(c, empty, sizeof(empty)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
