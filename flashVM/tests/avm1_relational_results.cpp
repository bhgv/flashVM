/* Numeric relational actions return undefined for unordered values, with distinct coercion order. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"

static unsigned calls, order[2];


static FVM_Result convert(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    order[calls++] = id;
    fvm_value_number(out, id);
    return FVM_OK;
}


static FVM_Value compare(FVM_Context *c, unsigned opcode, const FVM_Value &a, const FVM_Value &b)
{
    if (!(fvm_set_global(c, "left", &a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "right", &b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes body;
    global(body, "left");
    global(body, "right");
    body.push_back((unsigned char)opcode);
    return run(c, body);
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
        FVM_Value a, b;
        fvm_value_number(&a, std::numeric_limits<double>::quiet_NaN());
        fvm_value_number(&b, 1);
        if (!(compare(c, 0x48, a, b).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(compare(c, 0x67, a, b).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&a, "invalid");
        if (!(compare(c, 0x48, a, b).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(compare(c, 0x67, b, a).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&a, 1);
        if (!(compare(c, 0x48, a, b).type == FVM_VALUE_BOOLEAN && !compare(c, 0x48, a, b).boolean))
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
        fvm_value_undefined(&a);
        a.type = FVM_VALUE_OBJECT;
        a.object = left;
        b = a;
        b.object = right;
        calls = 0;
        if (!(compare(c, 0x48, a, b).boolean && calls == 2 && order[0] == 1 && order[1] == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        calls = 0;
        if (!(!compare(c, 0x67, a, b).boolean && calls == 2 && order[0] == 2 && order[1] == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
