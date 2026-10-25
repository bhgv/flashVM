/* SetVariable this mutates the activation cell, retaining arbitrary typed values. */
#define FVM_THIS_PATH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "anonymous_this_paths.cpp"


static FVM_Object *assigned_this_function(FVM_Context *c, const char *name, unsigned followup)
{
    Program body;
    body.text(name);
    body.variable("replacement");
    body.op(0x1d);
    if (!followup)
    {
        body.variable("this");
    }
    else if (followup == 1)
    {
        body.variable("this.marker");
    }
    else
    {
        body.number(0);
        body.variable("inheritedFunction");
        body.text("");
        body.op(0x52);
    }
    body.finish();
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
}
#ifndef FVM_THIS_ASSIGNMENT_FIXTURE_ONLY


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
        for (unsigned type = 0; type < 7; ++type)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                for (unsigned followup = 0; followup < 3; ++followup)
                {
                    if ((route >= 2 && !fvm_jit_available()) || (followup == 2 && version < 7))
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
                    FVM_Value replacement, out;
                    fvm_value_undefined(&replacement);
                    if (type == 1)
                    {
                        replacement.type = FVM_VALUE_NULL;
                    }
                    if (type == 2)
                    {
                        fvm_value_boolean(&replacement, 1);
                    }
                    if (type == 3)
                    {
                        fvm_value_number(&replacement, -17.5);
                    }
                    if (type == 4)
                    {
                        fvm_value_string_bytes(&replacement, "r\0s", 3);
                    }
                    if (type == 5 || type == 6)
                    {
                        FVM_Object *o = 0;
                        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        replacement = class_value(o);
                        fvm_value_number(&out, 81);
                        if (!(fvm_object_set(c, o, "marker", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (type == 6)
                        {
                            fvm_value_number(&replacement, 19);
                            replacement.metatable = o;
                        }
                    }
                    if (!(fvm_set_global(c, "replacement", &replacement) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (followup == 2)
                    {
                        out = class_value(undefined_this_function(c, 5));
                        if (!(fvm_set_global(c, "inheritedFunction", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    FVM_Object *fn = assigned_this_function(c, "this", followup);
                    out = this_path_invoke(c, fn, false, route);
                    if (version == 4)
                    {
                        if (!followup)
                        {
                            boxing_equal(out, replacement);
                        }
                    }
                    else if (followup != 1)
                    {
                        boxing_equal(out, replacement);
                    }
                    else if (type == 5)
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 81))
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
                    if (version >= 5 && followup != 1)
                    {
                        if (!(out.metatable == replacement.metatable))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->script_this_cell && !c->script_this && !c->script_this_undefined && !c->gc_frames &&
                          !c->script_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    for (unsigned version = 5; version <= 8; ++version)
    {
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
        FVM_Value value;
        fvm_value_number(&value, 27);
        if (!(fvm_set_global(c, "replacement", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = this_path_invoke(c, assigned_this_function(c, "THIS", 0), false, 0);
        if (!(value.type == FVM_VALUE_OBJECT && value.object == c->global && !c->script_this_cell))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
