/* Inherited getter-only virtual dispatch consumes a thrown watcher without creating own data. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"
#include <cstring>

static unsigned getter_only_calls;


static FVM_Result getter_only(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++getter_only_calls;
    fvm_value_number(out, 17);
    return FVM_OK;
}


static FVM_Result getter_only_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    if (!(frame->argc == 4 && frame->argv[1].type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_string_bytes(&value, "watch\0only", 10);
    return fvm_exception_raise(c, &value);
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
        for (unsigned own = 0; own < 2; ++own)
        {
            for (unsigned numeric = 0; numeric < 2; ++numeric)
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
                FVM_Object *object = 0, *parent = 0, *fn = 0;
                FVM_Value args[2], value, out;
                const char *key = numeric ? "0" : "slot";
                if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                      fvm_object_set_prototype(c, object, parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_define_accessor_rooted(c, own ? object : parent, key, getter_only, 0, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char end[] = {0};
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK &&
                      fvm_function_set_body(c, fn, getter_only_watch, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string(args, key);
                fvm_value_undefined(args + 1);
                args[1].type = FVM_VALUE_OBJECT;
                args[1].object = fn;
                if (!(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                getter_only_calls = 0;
                fvm_value_number(&value, 81);
                FVM_Result status = fvm_object_set(c, object, key, &value);
                if (!(status == (own ? FVM_EXCEPTION : FVM_OK) && !getter_only_calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (own)
                {
                    if (!(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 10 &&
                          !std::memcmp(out.string, "watch\0only", 10)))
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
                else
                {
                    if (!(!c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                int present = 0;
                if (!(fvm_object_has_own(c, object, key, &present) == FVM_OK && !!present == !!own))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, object, key, &out) == FVM_OK && out.number == 17 && getter_only_calls == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && c->watch_running.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
