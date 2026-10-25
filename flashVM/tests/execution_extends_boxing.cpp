/* Extends boxes superclass before subclass and reads the parent's prototype afterwards. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"

static unsigned sequence;


static FVM_Result number_box(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(sequence++ == 0 && fvm_gc_collect(c, 0) == FVM_OK && frame->argc == 1 && frame->argv[0].number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = frame->capture;
    if (!(fvm_object_set(c, frame->receiver, "prototype", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v.object = frame->receiver;
    if (!(fvm_set_global(c, "SuperBox", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result boolean_box(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(sequence++ == 1 && fvm_gc_collect(c, 0) == FVM_OK && frame->argc == 1 &&
          frame->argv[0].type == FVM_VALUE_BOOLEAN && frame->argv[0].boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value super, v;
    if (!(fvm_get_global(c, "SuperBox", &super) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = frame->capture;
    if (!(fvm_object_set(c, super.object, "prototype", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v.object = frame->receiver;
    if (!(fvm_set_global(c, "SubBox", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static void class_function(FVM_Context *c, const char *name, FVM_ScriptBody body, FVM_Object *capture)
{
    FVM_Object *fn = 0;
    FVM_Value v;
    const unsigned char code[] = {0};
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, body, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = fn;
    if (!(fvm_set_global(c, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
        FVM_Object *first = 0, *second = 0, *actual = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_object_create(c, &first) == FVM_OK && fvm_object_create(c, &second) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        class_function(c, "Number", number_box, first);
        class_function(c, "Boolean", boolean_box, second);
        FVM_Value sub, super, boxed_super, boxed_sub, child, ctor;
        fvm_value_boolean(&sub, 1);
        fvm_value_number(&super, 17);
        sequence = 0;
        if (!(fvm_exec_extends(c, &sub, &super) == FVM_OK && sequence == 2 &&
              fvm_get_global(c, "SuperBox", &boxed_super) == FVM_OK &&
              fvm_get_global(c, "SubBox", &boxed_sub) == FVM_OK &&
              fvm_object_get(c, boxed_sub.object, "prototype", &child) == FVM_OK &&
              fvm_object_get_prototype(c, child.object, &actual) == FVM_OK && actual == second &&
              fvm_object_get(c, child.object, "constructor", &ctor) == FVM_OK && ctor.object == boxed_super.object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_set_global(c, "SubOperand", &sub) == FVM_OK && fvm_set_global(c, "SuperOperand", &super) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes actions;
        variable(actions, "SubOperand");
        variable(actions, "SuperOperand");
        actions.push_back(0x69);
        actions.push_back(0);
        sequence = 0;
        if (!(fvm_avm1_execute(c, actions.begin(), actions.size()) == FVM_OK && sequence == 2 &&
              fvm_get_global(c, "SubBox", &boxed_sub) == FVM_OK &&
              fvm_object_get(c, boxed_sub.object, "prototype", &child) == FVM_OK &&
              fvm_object_get_prototype(c, child.object, &actual) == FVM_OK && actual == second))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
