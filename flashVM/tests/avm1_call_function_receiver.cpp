/* Loaded CallFunction uses the selected timeline rather than the global object. */
#define main named_scope_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2_named_scope.cpp"

#undef main
#include "../src/avm1/action_codes.h"
#include "../src/avm1/encoding_constants.h"


static void integer(Bytes &b, unsigned value)
{
    b.push_back(FVM_AVM1_ACTION_PUSH);
    u16(b, 5);
    b.push_back(FVM_AVM1_PUSH_INTEGER);
    for (unsigned i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)(value >> (8 * i)));
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
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
        FVM_Object *root = 0;
        if (!(fvm_construct(c, "MovieClip", 0, 0, &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes body;
        variable(body, "this");
        body.push_back(FVM_AVM1_ACTION_RETURN);
        body.push_back(FVM_AVM1_ACTION_END);
        FVM_Object *fn = 0;
        if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_set_global(c, "readThis", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes code;
        text(code, "observed");
        integer(code, 0);
        text(code, "readThis");
        code.push_back(FVM_AVM1_ACTION_CALL_FUNCTION);
        code.push_back(FVM_AVM1_ACTION_SET_VARIABLE);
        code.push_back(FVM_AVM1_ACTION_END);
        if (!(fvm_avm1_execute_receiver(c, code.begin(), code.size(), root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, root, "observed", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT &&
              value.object == root))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->script_this && !c->script_depth && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
