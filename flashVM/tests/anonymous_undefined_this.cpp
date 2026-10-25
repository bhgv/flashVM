/* Anonymous CallMethod passes Undefined this; Function2 preloads differ from its inherited local cell. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"


static FVM_Object *undefined_this_function(FVM_Context *c, unsigned kind)
{
    Program body;
    if (kind == 2 || kind == 4 || kind == 7)
    {
        body.op(0x96);
        body.op(2);
        body.op(0);
        body.op(4);
        body.op(1);
    }
    else
    {
        body.variable(kind == 6 || kind == 8 || kind == 9 ? "super" : "this");
    }
    body.finish();
    FVM_Object *function = 0;
    if (!kind || kind == 9)
    {
        if (!(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return function;
    }
    fvm_detail::vector<unsigned char> header, code;
    const char *name = "undefinedThisFunction2";
    for (size_t i = 0; i <= std::strlen(name); ++i)
    {
        header.push_back((unsigned char)name[i]);
    }
    header.push_back(0);
    header.push_back(0);
    header.push_back(2);
    unsigned flags = kind == 2 || kind == 5    ? FVM_AVM1_F2_PRELOAD_THIS
                     : kind == 3 || kind == 6  ? FVM_AVM1_F2_SUPPRESS_THIS
                     : kind == 4 || kind == 10 ? FVM_AVM1_F2_PRELOAD_THIS | FVM_AVM1_F2_SUPPRESS_THIS
                     : kind == 7               ? FVM_AVM1_F2_PRELOAD_SUPER
                                               : 0;
    header.push_back((unsigned char)flags);
    header.push_back((unsigned char)(flags >> 8));
    header.push_back((unsigned char)body.bytes.size());
    header.push_back((unsigned char)(body.bytes.size() >> 8));
    code.push_back(0x8e);
    code.push_back((unsigned char)header.size());
    code.push_back((unsigned char)(header.size() >> 8));
    for (size_t i = 0; i < header.size(); ++i)
    {
        code.push_back(header[i]);
    }
    for (size_t i = 0; i < body.bytes.size(); ++i)
    {
        code.push_back(body.bytes[i]);
    }
    code.push_back(0);
    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_get_global(c, name, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}
#ifndef FVM_UNDEFINED_THIS_FIXTURE_ONLY


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
        for (unsigned kind = 0; kind < 11; ++kind)
        {
            for (unsigned undefined_name = 0; undefined_name < 2; ++undefined_name)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    if ((kind && kind != 9 && version < 7) || (route >= 2 && !fvm_jit_available()))
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_load(c, version);
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value owner = class_value(undefined_this_function(c, kind)), name, out;
                    if (!(fvm_set_global(c, "anonymousFunction", &owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!route)
                    {
                        if (undefined_name)
                        {
                            fvm_value_undefined(&name);
                        }
                        else
                        {
                            fvm_value_string(&name, "");
                        }
                        if (!(fvm_exec_call_method(c, 0, &owner, &name, c->global, 0, 0, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        Program code;
                        code.number(0);
                        code.variable("anonymousFunction");
                        if (undefined_name)
                        {
                            code.variable("undefined");
                        }
                        else
                        {
                            code.text("");
                        }
                        code.op(0x52);
                        code.finish();
                        out = boxing_run(c, code, route - 1, FVM_OK);
                    }
                    if (kind == 3 || kind == 5 || kind == 10)
                    {
                        if (!(out.type == FVM_VALUE_OBJECT && out.object == c->global))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_UNDEFINED))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->calling_this_undefined && !c->script_this_undefined && !c->script_this && !c->gc_frames &&
                          !c->gc_native_depth && !c->script_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
#endif
