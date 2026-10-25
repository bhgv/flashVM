/* CastOp discards boxing output but preserves constructor side effects, GC roots and errors. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"

static FVM_Object *expected;
static unsigned boxes;
static bool fail_box;


static FVM_Result boxing(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Value v;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && frame->argc == 1 && frame->argv[0].number == 42 &&
          fvm_object_get(c, expected, "alive", &v) == FVM_OK && v.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++boxes;
    if (fail_box)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned v = 4; v <= 8; ++v)
    {
        FVM_Context *c = 0;
        FVM_Object *fn = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)v;
        const unsigned char code[] = {0};
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, boxing, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, input, klass, out;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_set_global(c, "Number", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned fail = 0; fail < 2; ++fail)
        {
            if (!(fvm_object_create(c, &expected) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 17);
            if (!(fvm_object_set(c, expected, "alive", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&klass);
            klass.type = FVM_VALUE_OBJECT;
            klass.object = expected;
            fvm_value_number(&input, 42);
            fvm_value_number(&out, 99);
            fail_box = fail != 0;
            FVM_Result r = fvm_exec_cast(c, &input, &klass, &out);
            if (fail)
            {
                if (!(r == FVM_EXCEPTION && out.number == 99 && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK && out.type == FVM_VALUE_NULL))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fail_box = false;
        if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expected = klass.object;
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, expected, "alive", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes actions;
        variable(actions, "Object");
        integer(actions, 42);
        actions.push_back(0x2b);
        actions.push_back(0x3e);
        actions.push_back(0);
        FVM_Object *action = 0;
        if (!(fvm_function_create_avm1(c, actions.begin(), actions.size(), 0, 0, 0, &action) == FVM_OK &&
              fvm_function_invoke(c, action, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NULL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(boxes == 15))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
