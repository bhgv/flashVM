/* Clearing the pending exception before a catch setter must not drop the thrown object's root. */
#define main exceptions_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "actionscript_exceptions.cpp"

#undef main
#define FVM_SETTER_COLLECTOR_FIXTURE_ONLY
#include "script_setter_collector.c"
#include "../src/core/runtime.hpp"


static FVM_Result fresh_exception(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(!n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *object = 0;
    FVM_Value value;
    if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, object, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}


static FVM_Result catch_setter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Result r = setter(c, frame, out, raw);
    return r == FVM_OK ? fvm_object_set(c, frame->capture, "last", frame->argv) : r;
}


static FVM_Result catch_getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    return fvm_object_get(c, frame->capture, "last", out);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_extensions_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *owner = 0, *plain = 0, *capture = 0, *fn = 0;
    if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK &&
          fvm_object_create(c, &capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value, out;
    fvm_value_number(&value, 99);
    if (!(fvm_object_set(c, capture, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char empty_code[] = {0};
    if (!(fvm_function_create_avm1(c, empty_code, sizeof(empty_code), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, catch_setter, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, empty_code, sizeof(empty_code), 0, 0, 0, &plain) == FVM_OK &&
          fvm_function_set_body(c, plain, catch_getter, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    install(c, owner, plain, fn);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = owner;
    if (!(fvm_set_global(c, "holder", &value) == FVM_OK &&
          fvm_register_function(c, "freshException", fresh_exception, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes body, caught, final, code;
    integer(body, 0);
    string(body, "freshException");
    body.push_back(0x3d);
    body.push_back(0x2a);
    method(caught, "gc", "collect");
    caught.push_back(0x17);
    string(caught, "observed");
    variable(caught, "holder:value");
    string(caught, "alive");
    caught.push_back(0x4e);
    caught.push_back(0x1d);
    assign(final, "finished", 1);
    attempt(code, body, caught, final, 3, "holder:value");
    run(c, code, FVM_OK);
    if (!(global(c, "observed") == 42 && global(c, "finished") == 1 && setter_entries == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 2;
    Bytes fail, mark, after;
    assign(mark, "mustNotRun", 1);
    assign(after, "mustNotFinalize", 1);
    attempt(fail, body, mark, after, 3, "holder:value");
    run(c, fail, FVM_EXCEPTION);
    if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 77 && fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "mustNotRun", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
          fvm_get_global(c, "mustNotFinalize", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->lexical_scopes.empty() && !c->gc_frames && !c->calling_setter_owner && !c->calling_setter_name &&
          !c->gc_native_depth && !c->script_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
