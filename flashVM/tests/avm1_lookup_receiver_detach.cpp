/* With classification is captured before a getter detaches the lexical chain and collects. */
#define main with_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/execution.h"


static void block(Bytes &b, const Bytes &body)
{
    b.push_back(0x94);
    u16(b, 2);
    u16(b, (unsigned)body.size());
    for (size_t i = 0; i < body.size(); ++i)
    {
        b.push_back(body[i]);
    }
}

static FVM_Object *detached_node, *expected_receiver;
static unsigned called;


static FVM_Result callable(FVM_Context *, FVM_Object *self, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(self == expected_receiver && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++called;
    fvm_value_number(out, 42);
    return FVM_OK;
}


static FVM_Result getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Object *node = detached_node;
    if (!node)
    {
        if (!(c->lexical_scopes.size() >= 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        node = c->lexical_scopes[c->lexical_scopes.size() - 2];
    }
    if (!(node->resource_kind == FVM_RESOURCE_WITH_SCOPE && fvm_object_set_prototype(c, node, 0) == FVM_OK &&
          fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = frame->capture;
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
    for (unsigned version = 5; version <= 8; ++version)
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
        FVM_Object *outer = 0, *inner = 0, *function = 0, *accessor = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &outer) == FVM_OK && fvm_construct(c, "Object", 0, 0, &inner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expected_receiver = outer;
        if (!(fvm_function_create_native(c, callable, 0, 0, &function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0};
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &accessor) == FVM_OK &&
              fvm_function_set_body(c, accessor, getter, 0, function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value args[3], value, out;
        fvm_value_string(args, "method");
        fvm_value_undefined(args + 1);
        args[1].type = FVM_VALUE_OBJECT;
        args[1].object = accessor;
        fvm_value_undefined(args + 2);
        args[2].type = FVM_VALUE_NULL;
        if (!(fvm_object_call(c, outer, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = outer;
        if (!(fvm_set_global(c, "outer", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = inner;
        if (!(fvm_set_global(c, "inner", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *outer_scope = 0, *inner_scope = 0;
        value.object = outer;
        if (!(fvm_exec_with_scope(c, 0, &value, &outer_scope) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = inner;
        if (!(fvm_exec_with_scope(c, outer_scope, &value, &inner_scope) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        detached_node = inner_scope;
        FVM_Value name;
        fvm_value_string(&name, "method");
        FVM_Object *owner = 0;
        int found = 0;
        if (!(fvm_exec_variable_lookup(c, inner_scope, 0, 0, 0, 1, &name, &out, &owner, &found) == FVM_OK && found &&
              out.object == function && owner == outer))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        detached_node = 0;
        Bytes body;
        integer(body, 0);
        text(body, "method");
        body.push_back(0x3d);
        body.push_back(0x3e);
        Bytes middle;
        variable(middle, "inner");
        block(middle, body);
        Bytes code;
        variable(code, "outer");
        block(code, middle);
        code.push_back(0);
        FVM_Object *fn = 0;
        if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK && out.number == 42 && c->lexical_scopes.empty() &&
              !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(called == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
