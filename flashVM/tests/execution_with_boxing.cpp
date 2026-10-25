/* With boxes through current global classes, retaining the parent across script construction. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"

static bool fail_box;


static FVM_Result box_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(fvm_gc_collect(c, 0) == FVM_OK && frame->argc == 1 && frame->argv[0].number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail_box)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    fvm_value_number(out, 73);
    FVM_Result r = fvm_object_set(c, frame->receiver, "cell", out);
    fvm_value_undefined(out);
    return r;
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
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *parent = 0, *fn = 0, *scope = 0, *owner = 0;
        FVM_Value input, value, key, out;
        int found = 0;
        if (!(fvm_object_create(c, &parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, parent, "outer", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char code[] = {0};
        if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, box_body, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_set_global(c, "Number", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, 42);
        if (!(fvm_exec_with_scope(c, parent, &input, &scope) == FVM_OK && scope != parent &&
              fvm_gc_pin(c, scope) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "cell");
        if (!(fvm_exec_scope_lookup(c, scope, &key, &out, &owner, &found) == FVM_OK && found && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "outer");
        if (!(fvm_exec_scope_lookup(c, scope, &key, &out, &owner, &found) == FVM_OK && found && out.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_box = true;
        FVM_Object *sentinel = scope;
        if (!(fvm_exec_with_scope(c, parent, &input, &scope) == FVM_EXCEPTION && scope == sentinel &&
              fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_box = false;
        fvm_value_undefined(&value);
        if (!(fvm_set_global(c, "Number", &value) == FVM_OK &&
              fvm_exec_with_scope(c, parent, &input, &scope) == FVM_OK && scope != parent))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "valueOf");
        if (!(fvm_exec_scope_lookup(c, scope, &key, &out, &owner, &found) == FVM_OK && !found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&input);
        if (!(fvm_exec_with_scope(c, parent, &input, &scope) == FVM_OK && scope == parent))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
