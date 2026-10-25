#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"

static unsigned count;
static unsigned order[4];


static FVM_Result convert(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{
    unsigned id = *(unsigned *)u;
    order[count++] = id;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, id == 1 ? 9 : 2);
    return FVM_OK;
}


static FVM_Result fail(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    return FVM_ERROR;
}


static FVM_Value operation(FVM_Context *c, unsigned op, const FVM_Value &a, const FVM_Value &b)
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
    Bytes bytes;
    global(bytes, "left");
    if (op != 0x50 && op != 0x51)
    {
        global(bytes, "right");
    }
    bytes.push_back((unsigned char)op);
    return run(c, bytes);
}


static FVM_Value number(double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    return v;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
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
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    struct Case
    {
        unsigned op;
        double a, b, result;
    };

    const Case cases[] = {{0x60, 0xffffffffUL, 0x80000001UL, -2147483647.0},
                          {0x61, 0x80000000UL, 1, -2147483647.0},
                          {0x62, 0xffffffffUL, 0x7fffffffUL, -2147483648.0},
                          {0x63, 0x40000001UL, 1, -2147483646.0},
                          {0x63, 0xffffffffUL, 32, -1},
                          {0x63, 1, 33, 2},
                          {0x63, 1, -1, -2147483648.0},
                          {0x64, 0x80000000UL, 1, -1073741824.0},
                          {0x64, 0x80000000UL, 31, -1},
                          {0x64, 0xffffffffUL, 0, -1},
                          {0x65, 0x80000000UL, 1, 1073741824.0},
                          {0x65, 0xffffffffUL, 0, 4294967295.0},
                          {0x65, -1, 31, 1},
                          {0x60, 4294967297.0, 3, 1},
                          {0x60, -1.9, 15, 15},
                          {0x3f, 9, 2, 1},
                          {0x3f, -9, 2, -1},
                          {0x3f, 9, -2, 1},
                          {0x50, 1.5, 0, 2.5},
                          {0x51, 1.5, 0, 0.5}};
    for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i)
    {
        FVM_Value out = operation(c, cases[i].op, number(cases[i].a), number(cases[i].b));
        if (!(out.type == FVM_VALUE_NUMBER && out.number == cases[i].result))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(operation(c, 0x67, number(3), number(2)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!operation(c, 0x67, number(2), number(3)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!operation(c, 0x67, number(std::numeric_limits<double>::quiet_NaN()), number(3)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value string_a, string_b;
    fvm_value_string(&string_a, "z");
    fvm_value_string(&string_b, "a");
    if (!(operation(c, 0x67, string_a, string_b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value text_value;
    fvm_value_string(&text_value, "4294967297");
    if (!(operation(c, 0x63, text_value, number(1)).number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&text_value, "1.5");
    if (!(operation(c, 0x50, text_value, number(0)).number == 2.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out = operation(c, 0x3f, number(1), number(0));
    if (!(out.number != out.number))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
    out = operation(c, 0x3f, number(inf), number(2));
    if (!(out.number != out.number))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(operation(c, 0x3f, number(-7), number(inf)).number == -7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(operation(c, 0x60, number(nan), number(3)).number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(operation(c, 0x61, number(inf), number(3)).number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    out = operation(c, 0x3f, number(-0.0), number(2));
    if (!(out.number == 0 && 1.0 / out.number < 0))
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
    FVM_Value a, b;
    fvm_value_undefined(&a);
    a.type = FVM_VALUE_OBJECT;
    a.object = left;
    b = a;
    b.object = right;
    count = 0;
    if (!(operation(c, 0x3f, a, b).number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(count == 2 && order[0] == 2 && order[1] == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    count = 0;
    if (!(operation(c, 0x67, a, b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(count == 2 && order[0] == 2 && order[1] == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Legacy comparison coerces numeric strings rather than comparing them lexically. */
    fvm_value_string(&string_a, "10");
    fvm_value_string(&string_b, "2");
    if (!(!operation(c, 0x0f, string_a, string_b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(operation(c, 0x0e, string_a, number(10)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&string_a, "invalid");
    if (!(!operation(c, 0x0e, string_a, string_a).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!operation(c, 0x10, number(nan), number(1)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(operation(c, 0x11, number(nan), number(1)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&string_a, 1);
    if (!(operation(c, 0x10, string_a, number(1)).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    count = 0;
    if (!(!operation(c, 0x0e, a, b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(count == 2 && order[0] == 2 && order[1] == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    count = 0;
    if (!(!operation(c, 0x0f, a, b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(count == 2 && order[0] == 1 && order[1] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    count = 0;
    if (!(operation(c, 0x10, a, b).boolean && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, left, "valueOf", fail, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes broken;
    global(broken, "left");
    broken.push_back(0x50);
    broken.push_back(0);
    if (!(fvm_avm1_execute(c, broken.begin(), broken.size()) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(operation(c, 0x50, number(1), number(0)).number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned op = 0x0e; op <= 0x0f; ++op)
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
        Bytes bad;
        global(bad, "left");
        global(bad, "right");
        bad.push_back((unsigned char)op);
        bad.push_back(0);
        count = 0;
        if (!(fvm_avm1_execute(c, bad.begin(), bad.size()) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(count == (op == 0x0e ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(operation(c, op, number(1), number(1)).boolean == (op == 0x0e)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned op = 0x60; op <= 0x65; ++op)
    {
        unsigned char underflow[] = {(unsigned char)op, 0};
        if (!(fvm_avm1_execute(c, underflow, sizeof(underflow)) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
