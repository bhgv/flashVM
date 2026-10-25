/* AVM1 numeric grammar/version gates differ from the host C conversion API. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"


static FVM_Value numeric(FVM_Context *c, const FVM_Value &value)
{
    if (!(fvm_set_global(c, "input", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    global(b, "input");
    b.push_back(0x4a);
    return run(c, b);
}


static FVM_Result converted(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    return fvm_value_string(out, "0xffffffff");
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
    const char *samples[] = {"010",
                             "-010",
                             "0xffffffff",
                             "0x-10",
                             "0x+10",
                             "+0x10",
                             " 010",
                             "010.5",
                             "Infinity",
                             "",
                             "1 ",
                             "1e",
                             "1e+",
                             "1e4294967296",
                             "-.",
                             "\r10"};
    const double expected[] = {8, -8, -1, -16, 16, 0, 10, 10.5, 0, 0, 0, 1, 1, 1, 0, 10};
    for (unsigned version = 5; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 0, 16, 0, 0, 0, 0x08, 0, 0, 12, 1, 0, 0, 0};
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
        for (unsigned i = 0; i < sizeof(samples) / sizeof(*samples); ++i)
        {
            FVM_Value value;
            fvm_value_string(&value, samples[i]);
            FVM_Value result = numeric(c, value);
            bool nan = i == 5 || i == 8 || i == 9 || i == 10 || (version == 5 && i >= 2 && i <= 4);
            double number = version == 5 && i == 0 ? 10 : version == 5 && i == 1 ? -10 : expected[i];
            if (!(result.type == FVM_VALUE_NUMBER))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(nan ? result.number != result.number : result.number == number))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (i == 14)
            {
                if (!(1.0 / result.number < 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        FVM_Value result = numeric(c, value);
        if (!(version < 7 ? result.number == 0 : result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.type = FVM_VALUE_NULL;
        result = numeric(c, value);
        if (!(version < 7 ? result.number == 0 : result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, std::numeric_limits<double>::infinity());
        if (!(numeric(c, value).number == value.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&value, "1\0x", 3);
        result = numeric(c, value);
        if (!(result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *o = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, o, "valueOf", converted, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = o;
        result = numeric(c, value);
        if (!(version == 5 ? result.number != result.number : result.number == -1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* The embedding conversion retains its independent documented grammar. */
        double host = 0;
        fvm_value_string(&value, "Infinity");
        if (!(fvm_value_to_number(c, &value, &host) == FVM_OK && host == std::numeric_limits<double>::infinity()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
