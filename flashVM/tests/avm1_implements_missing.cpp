/* Missing ImplementsOp count is zero: no primitive boxing or prototype snapshot publication. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned boxed;


static FVM_Result box_trap(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++boxed;
    fvm_value_undefined(out);
    return FVM_ERROR;
}


static void finish(Bytes &code)
{
    text(code, "continued");
    integer(code, 7);
    code.push_back(0x1d);
    code.push_back(0);
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
    for (unsigned profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)(profile + 3);
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
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
        FVM_Object *klass = 0, *prototype = 0, *trap = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &klass) == FVM_OK && fvm_object_create(c, &prototype) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = prototype;
        if (!(fvm_object_set(c, klass, "prototype", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = klass;
        if (!(fvm_set_global(c, "klass", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0};
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &trap) == FVM_OK &&
              fvm_function_set_body(c, trap, box_trap, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = trap;
        if (!(fvm_set_global(c, "Number", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned provided = 0; provided < 3; ++provided)
        {
            Bytes code;
            if (provided == 1)
            {
                integer(code, 7);
            }
            else if (provided == 2)
            {
                text(code, "klass");
                code.push_back(0x1c);
            }
            code.push_back(0x2c);
            finish(code);
            boxed = 0;
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Result r = fvm_avm1_execute(c, code.begin(), code.size());
            if (!(c->gc_frames == roots && !boxed && !prototype->interfaces_are_prototypes &&
                  prototype->implemented_interfaces.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (profile)
            {
                FVM_Value out;
                if (!(r == FVM_OK && fvm_get_global(c, "continued", &out) == FVM_OK && out.number == 7))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_ERROR))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
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
