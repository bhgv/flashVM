/* Implicit this precedes local/formal names in loaded SWF5+ calls. */
#define main named_scope_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2_named_scope.cpp"

#undef main


static FVM_Object *function(FVM_Context *c, bool function2, bool formal, bool local)
{
    Bytes body;
    if (local)
    {
        text(body, "this");
        text(body, "local-value");
        body.push_back(0x3c);
    }
    variable(body, "this");
    body.push_back(0x3e);
    body.push_back(0);
    FVM_Object *fn = 0;
    const char *parameters[] = {"this"};
    if (!function2)
    {
        if (!(fvm_function_create_avm1(c, body.begin(), body.size(), formal ? 1 : 0, parameters, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return fn;
    }
    Bytes header;
    name(header, "");
    u16(header, formal ? 1 : 0);
    header.push_back(0);
    u16(header, 0);
    if (formal)
    {
        header.push_back(0);
        name(header, "this");
    }
    u16(header, (unsigned)body.size());
    Bytes outer;
    outer.push_back(0x8e);
    u16(outer, (unsigned)header.size());
    for (size_t i = 0; i < header.size(); ++i)
    {
        outer.push_back(header[i]);
    }
    for (size_t i = 0; i < body.size(); ++i)
    {
        outer.push_back(body[i]);
    }
    outer.push_back(0x3e);
    outer.push_back(0);
    FVM_Object *factory = 0;
    if (!(fvm_function_create_avm1(c, outer.begin(), outer.size(), 0, 0, 0, &factory) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_function_invoke(c, factory, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
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
    for (unsigned version = 4; version <= 9; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
            swf[3] = (unsigned char)version;
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
        FVM_Object *self = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned kind = 0; kind < 2; ++kind)
        {
            for (unsigned formal = 0; formal < 2; ++formal)
            {
                for (unsigned local = 0; local < 2; ++local)
                {
                    FVM_Object *fn = function(c, kind != 0, formal != 0, local != 0);
                    if (!(fvm_gc_pin(c, fn) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value arg, out;
                    fvm_value_string(&arg, "formal-value");
                    if (!(fvm_function_invoke(c, fn, self, 1, &arg, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if ((version >= 5 && version <= 8) || (!formal && !local))
                    {
                        if (!(out.type == FVM_VALUE_OBJECT && out.object == self))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_STRING &&
                              !std::strcmp(out.string, local ? "local-value" : "formal-value")))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_unpin(c, fn) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
