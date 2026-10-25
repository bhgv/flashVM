/* SWF 8 URShift returns a signed Int32; earlier supported versions return Uint32. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"

static unsigned calls, order[2];


static FVM_Result convert(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    order[calls++] = id;
    fvm_value_number(out, id == 1 ? -1 : 0);
    return FVM_OK;
}


static FVM_Result fail(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    return FVM_ERROR;
}


static FVM_Value shift(FVM_Context *c, unsigned op, double left, double right)
{
    FVM_Value value;
    fvm_value_number(&value, left);
    if (!(fvm_set_global(c, "left", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, right);
    if (!(fvm_set_global(c, "right", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    global(b, "left");
    global(b, "right");
    b.push_back((unsigned char)op);
    return run(c, b);
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
        if (!(shift(c, 0x65, -1, 0).number == (version == 8 ? -1 : 4294967295.0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(shift(c, 0x65, -3, 32).number == (version == 8 ? -3 : 4294967293.0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(shift(c, 0x65, -3, 1).number == 2147483646.0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(shift(c, 0x65, 2147483648.0, 0).number == (version == 8 ? -2147483648.0 : 2147483648.0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(shift(c, 0x64, -3, 0).number == -3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *left = 0, *right = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &left) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &right) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned ids[] = {1, 2};
        if (!(fvm_object_define_method_rooted(c, left, "valueOf", convert, ids, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, right, "valueOf", convert, ids + 1, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = left;
        if (!(fvm_set_global(c, "left", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = right;
        if (!(fvm_set_global(c, "right", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned op = 0x60; op <= 0x65; ++op)
        {
            Bytes b;
            global(b, "left");
            global(b, "right");
            b.push_back((unsigned char)op);
            calls = 0;
            run(c, b);
            if (!(calls == 2 && order[0] == 2 && order[1] == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_define_method_rooted(c, right, "valueOf", fail, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes b;
        global(b, "left");
        global(b, "right");
        b.push_back(0x65);
        b.push_back(0);
        calls = 0;
        if (!(fvm_avm1_execute(c, b.begin(), b.size()) == FVM_ERROR && calls == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
