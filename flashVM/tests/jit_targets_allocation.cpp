/* Native target literal compilation, dynamic coercion and dispatch restore state on allocation failure. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_TARGET_FIXTURE_ONLY
#include "jit_targets.cpp"


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
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (unsigned dynamic = 0; dynamic < 2; ++dynamic)
        {
            FVM_Context *c = 0;
            const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (loaded)
            {
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Object *root = target_setup(c), *child = target_child(c, root, "child", 2), *coercion = 0,
                       *conversion = 0, *fn = 0;
            FVM_Value value, out;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = child;
            if (!(fvm_object_set(c, root, "alias", &value) == FVM_OK && fvm_object_create(c, &coercion) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, coercion) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned char empty[] = {0};
            if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &conversion) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, conversion, target_path, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value.object = conversion;
            if (!(fvm_object_set(c, coercion, "toString", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program p;
            if (dynamic)
            {
                p.variable("input");
                p.op(0x20);
            }
            else
            {
                target_literal(p, "alias");
            }
            target_finish(p);
            FVM_JitCode *code = 0;
            bool compiled = false;
            unsigned compile_failures = 0;
            for (long boundary = 0; boundary < 2000 && !compiled; ++boundary)
            {
                allowance = boundary;
                FVM_Result status = fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code);
                allowance = -1;
                if (status == FVM_OK)
                {
                    compiled = true;
                }
                else
                {
                    if (!(status == FVM_OUT_OF_MEMORY && !code))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++compile_failures;
                }
            }
            if (!(compiled && compile_failures))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const char *params[] = {"input"};
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 1, params, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, fn) == FVM_OK && fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool success = false;
            unsigned failures = 0;
            value.object = coercion;
            c->avm1_target_active = true;
            c->avm1_base_clip = root;
            c->avm1_target_clip = root;
            base_expected = root;
            target_expected = child;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result status = fvm_function_invoke(c, fn, root, 1, &value, &out);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && !code->active &&
                      c->avm1_target_active && c->avm1_base_clip == root && c->avm1_target_clip == root))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OK)
                {
                    success = true;
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OUT_OF_MEMORY))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(success && failures && fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_jit_destroy(code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
