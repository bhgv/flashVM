/* Qualified this paths must use the script cell rather than its embedding receiver. */
#define FVM_UNDEFINED_THIS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "anonymous_undefined_this.cpp"


static void this_path_program(Program &body, const char *path, unsigned operation)
{
    if (operation == 1)
    {
        body.text(path);
        body.number(73);
        body.op(0x1d);
        body.number(1);
    }
    else if (operation == 2)
    {
        body.text(path);
        body.op(0x3b);
    }
    else
    {
        body.variable(path);
    }
    body.finish();
}


static FVM_Object *this_path_function(FVM_Context *c, const char *path, unsigned operation)
{
    Program body;
    this_path_program(body, path, operation);
    FVM_Object *function = 0;
    if (!(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return function;
}


static FVM_Value this_path_invoke(FVM_Context *c,
                                  FVM_Object *function,
                                  bool anonymous,
                                  unsigned route,
                                  FVM_Result expected = FVM_OK,
                                  const char *path = 0,
                                  unsigned operation = 0)
{
    FVM_Value value = class_value(function), name, out;
    if (!(fvm_set_global(c, "pathFunction", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_JitCode *inner = 0;
    if (route == 2 && path)
    {
        Program body;
        this_path_program(body, path, operation);
        if (!(fvm_gc_pin(c, function) == FVM_OK &&
              fvm_jit_compile(body.bytes.begin(), body.bytes.size(), &inner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, function, fvm_jit_body, inner, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const unsigned long entries = c->execution_mode.stats.native_entries;
    if (!route)
    {
        if (anonymous)
        {
            fvm_value_string(&name, "");
            if (!(fvm_exec_call_method(c, 0, &value, &name, c->global, 0, 0, &out) == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_object_call(c, c->global, "pathFunction", 0, 0, &out) == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        Program code;
        code.number(0);
        code.variable(anonymous ? "pathFunction" : "this");
        code.text(anonymous ? "" : "pathFunction");
        code.op(0x52);
        code.finish();
        out = boxing_run(c, code, route - 1, expected);
    }
    if (inner)
    {
        if (!(!inner->active && c->execution_mode.stats.native_entries >= entries + 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, function, 0, 0, 0) == FVM_OK && fvm_jit_destroy(inner) == FVM_OK &&
              fvm_gc_unpin(c, function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return out;
}
#ifndef FVM_THIS_PATH_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *paths[] = {"this.marker", "this:marker", "this.child.marker", "this:child:marker"};
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned anonymous = 0; anonymous < 2; ++anonymous)
        {
            for (unsigned operation = 0; operation < 3; ++operation)
            {
                for (unsigned path = 0; path < 4; ++path)
                {
                    for (unsigned transport = 0; transport < 2; ++transport)
                    {
                        for (unsigned route = 0; route < 4; ++route)
                        {
                            if ((version && version < 5) || (route >= 2 && !fvm_jit_available()))
                            {
                                continue;
                            }
                            FVM_Context *c = 0, *source = 0;
                            if (!(fvm_context_create(rt, &c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (version)
                            {
                                version_load(c, version);
                            }
                            if (!(fvm_standard_install(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *child = 0;
                            if (!(fvm_object_create(c, &child) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value value = class_value(child), out;
                            if (!(fvm_set_global(c, "child", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_value_number(&value, 41);
                            if (!(fvm_set_global(c, "marker", &value) == FVM_OK &&
                                  fvm_object_set(c, child, "marker", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *function = 0;
                            if (transport)
                            {
                                if (!(fvm_context_create(rt, &source) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (version)
                                {
                                    version_load(source, version);
                                }
                                if (!(fvm_standard_install(source) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                value = class_value(this_path_function(source, paths[path], operation));
                                unsigned char *wire = 0;
                                size_t size = 0;
                                if (!(fvm_value_serialize(source, &value, &wire, &size) == FVM_OK &&
                                      fvm_value_deserialize(c, wire, size, &value) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                fvm_serialized_free(wire);
                                function = value.object;
                            }
                            else
                            {
                                function = this_path_function(c, paths[path], operation);
                            }
                            out = this_path_invoke(c, function, anonymous != 0, route, FVM_OK, paths[path], operation);
                            bool absent_this = version && anonymous;
                            if (!operation)
                            {
                                if (absent_this)
                                {
                                    if (!(out.type == FVM_VALUE_UNDEFINED))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 41))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                            if (operation == 2)
                            {
                                if (!(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == !absent_this))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            FVM_Object *target = path < 2 ? c->global : child;
                            if (!(fvm_object_get(c, target, "marker", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (operation == 2 && !absent_this)
                            {
                                if (!(value.type == FVM_VALUE_UNDEFINED))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                if (!(value.type == FVM_VALUE_NUMBER &&
                                      value.number == (operation == 1 && !absent_this ? 73 : 41)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!(!c->calling_this_undefined && !c->script_this_undefined && !c->gc_frames &&
                                  !c->script_depth))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_gc_collect(c, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_context_destroy(c);
                            if (source)
                            {
                                fvm_context_destroy(source);
                            }
                        }
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
#endif
