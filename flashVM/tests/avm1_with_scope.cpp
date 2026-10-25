/* With scopes retain captured objects through return, collection and graph transfer. */
#define main named_scope_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2_named_scope.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/exception.h"
#include "../src/avm1/action_codes.h"


static void block(Bytes &b, const Bytes &body)
{
    b.push_back(FVM_AVM1_ACTION_WITH);
    u16(b, 2);
    u16(b, (unsigned)body.size());
    for (size_t i = 0; i < body.size(); ++i)
    {
        b.push_back(body[i]);
    }
}


static Bytes make_body(bool throwing)
{
    Bytes body;
    if (throwing)
    {
        text(body, "failure");
        body.push_back(FVM_AVM1_ACTION_THROW);
        return body;
    }
    Bytes inner;
    variable(inner, "value");
    inner.push_back(FVM_AVM1_ACTION_RETURN);
    Bytes header;
    name(header, "");
    u16(header, 0);
    u16(header, (unsigned)inner.size());
    body.push_back(FVM_AVM1_ACTION_DEFINE_FUNCTION);
    u16(body, (unsigned)header.size());
    for (size_t i = 0; i < header.size(); ++i)
    {
        body.push_back(header[i]);
    }
    for (size_t i = 0; i < inner.size(); ++i)
    {
        body.push_back(inner[i]);
    }
    body.push_back(FVM_AVM1_ACTION_RETURN);
    return body;
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
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *object = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_number(&value, 42);
        if (!(fvm_object_set(c, object, "value", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        if (!(fvm_set_global(c, "scopeObject", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned throwing = 0; throwing < 2; ++throwing)
        {
            Bytes code;
            variable(code, "scopeObject");
            block(code, make_body(throwing != 0));
            code.push_back(FVM_AVM1_ACTION_END);
            FVM_Object *factory = 0;
            if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 0, 0, 0, &factory) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value out;
            FVM_Result status = fvm_function_invoke(c, factory, 0, 0, 0, &out);
            if (!(c->lexical_scopes.empty() && !c->gc_frames && !c->script_this))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (throwing)
            {
                if (!(status == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            if (!(status == FVM_OK && out.type == FVM_VALUE_OBJECT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *closure = out.object;
            if (!(fvm_gc_pin(c, closure) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Channel *channel = 0;
            if (!(fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_channel_send_value(c, channel, &out, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *copy = out.object;
            if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_invoke(c, closure, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                  out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_invoke(other, copy, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                  out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(channel);
        }
        FVM_Object *scope = 0;
        if (!(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, scope) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *owner = 0;
        int found = 0;
        FVM_Value key, out;
        fvm_value_string(&key, "value");
        if (!(fvm_exec_scope_lookup(c, scope, &key, &out, &owner, &found) == FVM_OK && found && owner == object &&
              out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "absent");
        if (!(fvm_exec_scope_lookup(c, scope, &key, &out, &owner, &found) == FVM_OK && !found && !owner))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *boxed = 0;
        fvm_value_string(&key, "abc");
        if (!(fvm_exec_with_scope(c, scope, &key, &boxed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "length");
        if (!(fvm_exec_scope_lookup(c, boxed, &key, &out, &owner, &found) == FVM_OK && found && out.number == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "value");
        if (!(fvm_exec_scope_lookup(c, boxed, &key, &out, &owner, &found) == FVM_OK && found && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&key);
        FVM_Object *unchanged = 0;
        if (!(fvm_exec_with_scope(c, scope, &key, &unchanged) == FVM_OK && unchanged == scope))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value undef;
        fvm_value_undefined(&undef);
        if (!(fvm_object_set(c, object, "undefinedBinding", &undef) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "undefinedBinding");
        if (!(fvm_exec_scope_lookup(c, scope, &key, &out, &owner, &found) == FVM_OK && found &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes assigning;
        text(assigning, "value");
        text(assigning, "updated");
        assigning.push_back(FVM_AVM1_ACTION_SET_VARIABLE);
        Bytes code;
        variable(code, "scopeObject");
        block(code, assigning);
        code.push_back(FVM_AVM1_ACTION_END);
        if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, object, "value", &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
              !std::strcmp(out.string, "updated")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        code.pop_back();
        code.pop_back();
        FVM_AVM1Info info;
        info.struct_size = sizeof(info);
        if (!(fvm_avm1_inspect(code.begin(), code.size(), &info) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
