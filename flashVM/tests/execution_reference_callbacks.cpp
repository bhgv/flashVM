/* Coercion roots the original scope, explicit receiver and name through GC and exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"

static const char *reference_text;


static FVM_Result reference_body(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failing)
    {
        FVM_Value e;
        fvm_value_number(&e, 77);
        return fvm_exception_raise(c, &e);
    }
    return fvm_value_string(out, reference_text);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *scope = 0, *key = 0, *leaf = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &scope) == FVM_OK && fvm_object_create(c, &key) == FVM_OK &&
          fvm_object_create(c, &leaf) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    attach(c, key, "toString", reference_body, 0);
    FVM_Value name, value, out;
    fvm_value_undefined(&name);
    name.type = FVM_VALUE_OBJECT;
    name.object = key;
    value = name;
    value.object = leaf;
    if (!(fvm_object_set(c, scope, "child", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned variant = 0; variant < 2; ++variant)
    {
        reference_text = variant ? "this.victim" : "child.victim";
        if (variant)
        {
            int removed = 0;
            if (!(fvm_object_delete(c, scope, "child", &removed) == FVM_OK && removed))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, leaf, "victim", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_delete_reference(c, scope, 0, leaf, &name, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, leaf, "victim", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failing = true;
        fvm_value_number(&out, 99);
        if (!(fvm_exec_delete_reference(c, scope, 0, leaf, &name, &out) == FVM_EXCEPTION && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_clear(c) == FVM_OK && fvm_object_get(c, leaf, "victim", &out) == FVM_OK &&
              out.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failing = false;
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
