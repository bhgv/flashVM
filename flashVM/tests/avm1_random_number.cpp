/* RandomNumber action shares Math state, retains coercion receivers and propagates exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static bool raising;
static unsigned conversions;


static FVM_Result bound_value(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++conversions;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v, math;
    if (!(fvm_object_get(c, frame->receiver, "alive", &v) == FVM_OK && v.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "Math", &math) == FVM_OK && fvm_object_call(c, math.object, "random", 0, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, raising ? 77 : 10);
    return raising ? fvm_exception_raise(c, out) : FVM_OK;
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        FVM_Object *object = 0;
        FVM_Value input, out;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, 17);
        if (!(fvm_object_set(c, object, "alive", &input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        attach(c, object, "valueOf", bound_value, 0);
        fvm_value_undefined(&input);
        input.type = FVM_VALUE_OBJECT;
        input.object = object;
        if (!(fvm_set_global(c, "bound", &input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes code;
        text(code, "result");
        variable(code, "bound");
        code.push_back(0x30);
        code.push_back(0x1d);
        code.push_back(0);
        raising = false;
        FVM_GCRootFrame *roots = c->gc_frames;
        if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK && c->random_state == 1586005467UL &&
              c->gc_frames == roots))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&out);
        if (!(fvm_set_global(c, "bound", &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        raising = true;
        fvm_value_number(&out, 99);
        if (!(fvm_exec_random_number(c, &input, &out) == FVM_EXCEPTION && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->random_state == 2165703038UL && c->gc_frames == roots && fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        raising = false;
        Bytes empty;
        empty.push_back(0x30);
        text(empty, "empty_result");
        empty.push_back(0x4d);
        empty.push_back(0x1d);
        empty.push_back(0);
        if (!(fvm_avm1_execute(c, empty.begin(), empty.size()) == FVM_OK && c->random_state == 2165703038UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "empty_result", &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(conversions == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
