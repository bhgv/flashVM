/* Invalid this paths skip accessors; named paths retain detached accessor/capture roots. */
#define FVM_THIS_PATH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "anonymous_this_paths.cpp"
#include "flashvm/exception.h"

static FVM_Object *path_target, *path_capture;
static unsigned path_calls, path_failure;


static FVM_Result path_accessor(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++path_calls;
    if (!(frame->receiver == path_target && frame->implicit_this == path_target))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->calling_this_undefined && !c->script_this_undefined && c->script_this == path_target))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, path_target, "marker", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "pathFunction", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "child", &removed) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, path_target, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool target_alive = false, capture_alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        target_alive |= c->objects[i] == path_target;
        capture_alive |= c->objects[i] == path_capture;
    }
    if (!(target_alive && capture_alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 41);
    if (path_failure == 1)
    {
        return fvm_exception_raise(c, out);
    }
    if (path_failure == 2)
    {
        return FVM_ERROR;
    }
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
        for (unsigned anonymous = 0; anonymous < 2; ++anonymous)
        {
            for (unsigned deep = 0; deep < 2; ++deep)
            {
                for (unsigned setter = 0; setter < 2; ++setter)
                {
                    for (path_failure = 0; path_failure < 3; ++path_failure)
                    {
                        for (unsigned route = 0; route < 4; ++route)
                        {
                            if (route >= 2 && !fvm_jit_available())
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
                            path_target = c->global;
                            if (deep)
                            {
                                if (!(fvm_construct(c, "Object", 0, 0, &path_target) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            FVM_Value value = class_value(path_target);
                            if (!(fvm_set_global(c, "child", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_construct(c, "Object", 0, 0, &path_capture) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *accessor = undefined_this_function(c, 0);
                            if (!(fvm_function_set_body(c, accessor, path_accessor, 0, path_capture) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value args[3], out;
                            fvm_value_string(args, "marker");
                            args[1] = args[2] = class_value(accessor);
                            FVM_Value constructor, prototype, method;
                            if (!(fvm_get_global(c, "Object", &constructor) == FVM_OK &&
                                  fvm_object_get(c, constructor.object, "prototype", &prototype) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_object_get(c, prototype.object, "addProperty", &method) == FVM_OK &&
                                  method.type == FVM_VALUE_OBJECT))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_function_invoke(c, method.object, path_target, 3, args, &out) == FVM_OK &&
                                  out.boolean))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *function =
                                this_path_function(c, deep ? "this.child.marker" : "this:marker", setter);
                            path_calls = 0;
                            const FVM_Result expected = anonymous || !path_failure ? FVM_OK
                                                        : path_failure == 1        ? FVM_EXCEPTION
                                                                                   : FVM_ERROR;
                            out = this_path_invoke(c,
                                                   function,
                                                   anonymous != 0,
                                                   route,
                                                   expected,
                                                   deep ? "this.child.marker" : "this:marker",
                                                   setter);
                            if (!route && expected == FVM_EXCEPTION)
                            {
                                if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 41))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!(fvm_exception_clear(c) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!(path_calls == (anonymous ? 0U : 1U)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (anonymous)
                            {
                                if (setter)
                                {
                                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 1))
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
                            }
                            if (!(!c->calling_this_undefined && !c->script_this_undefined && !c->script_this &&
                                  !c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_context_destroy(c);
                        }
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
