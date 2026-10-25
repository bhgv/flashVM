/* Real AVM1 onEnterFrame superclass calls retain inherited owner in interpreter/manual/auto JIT. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "movieclip_tick_fixture.hpp"


static FVM_Result answer(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(out, 42);
    return FVM_OK;
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
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            FVM_Context *c = tick_context(rt, version);
            FVM_Object *root = tick_root(c), *middle = 0, *owner = 0, *base = 0, *fn = 0;
            FVM_JitCode *native = 0;
            if (!(fvm_object_create(c, &middle) == FVM_OK && fvm_object_create(c, &owner) == FVM_OK &&
                  fvm_object_create(c, &base) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set_prototype(c, root, middle) == FVM_OK &&
                  fvm_object_set_prototype(c, middle, owner) == FVM_OK &&
                  fvm_object_set_prototype(c, owner, base) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(c, base, "answer", answer, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program p;
            p.variable("this");
            p.text("seen");
            p.number(0);
            p.variable("super");
            p.text("answer");
            p.op(0x52);
            p.op(0x4f);
            p.op(0);
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value = tick_object(fn);
            if (!(fvm_object_set(c, owner, "onEnterFrame", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (mode == 1)
            {
                if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &native) == FVM_OK &&
                      fvm_function_set_body(c, fn, fvm_jit_body, native, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            unsigned long before = c->execution_mode.stats.native_entries;
            if (!(fvm_movieclip_advance(c, root, 2) == FVM_OK &&
                  c->execution_mode.stats.native_entries == before + (mode ? 2 : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, root, "seen", &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
                  value.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            if (native)
            {
                if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(native) == FVM_OK))
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
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
