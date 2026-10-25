/* Folded property paths keep temporary objects alive across collecting script getters. */
#define FVM_UNICODE_PATH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_unicode_paths.cpp"
#include "flashvm/exception.h"

static FVM_Object *unicode_transient, *unicode_destination, *unicode_start;
static unsigned unicode_getters, unicode_failure;


static FVM_Value unicode_object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}


static void unicode_accessor(FVM_Context *c, FVM_Object *o, const char *name, FVM_Object *getter)
{
    FVM_Value args[3], out;
    fvm_value_string(args, name);
    args[1] = unicode_object(getter);
    fvm_value_undefined(args + 2);
    args[2].type = FVM_VALUE_NULL;
    if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result unicode_collect(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++unicode_getters;
    if (!(frame->receiver == unicode_transient && c->gc_native_depth == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *garbage = 0;
    if (!(fvm_object_create(c, &garbage) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long reclaimed = 0;
    if (!(fvm_gc_collect(c, &reclaimed) == FVM_OK && reclaimed > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value alive;
    if (!(fvm_object_get(c, unicode_transient, "alive", &alive) == FVM_OK && alive.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (unicode_failure == 1)
    {
        fvm_value_number(out, 81);
        return fvm_exception_raise(c, out);
    }
    if (unicode_failure == 2)
    {
        return FVM_ERROR;
    }
    *out = unicode_object(unicode_destination);
    return FVM_OK;
}


static FVM_Result unicode_factory(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++unicode_getters;
    if (!(frame->capture && frame->receiver == unicode_start && c->gc_native_depth == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &unicode_transient) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, unicode_transient, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unicode_accessor(c, unicode_transient, "\xd0\x94", frame->capture);
    *out = unicode_object(unicode_transient);
    return FVM_OK;
}


static FVM_Object *unicode_body(FVM_Context *c, FVM_ScriptBody body, FVM_Object *capture)
{
    const unsigned char empty[] = {0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, body, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
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
        for (unicode_failure = 0; unicode_failure < 3; ++unicode_failure)
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
            if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *root = 0;
            if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unicode_start = root;
            unicode_destination = child(c, root, "destination", 1);
            FVM_Object *collector = unicode_body(c, unicode_collect, 0);
            unicode_accessor(
                c, version % 2 ? root : root->prototype, "\xc3\x84", unicode_body(c, unicode_factory, collector));
            FVM_Value path;
            fvm_value_string(&path, version < 7 ? "\xc3\xa4/\xd0\xb4" : "\xc3\x84/\xd0\x94");
            FVM_Object *out = root;
            unicode_getters = 0;
            if (!(fvm_movieclip_resolve_path(c, root, path, &out) == (unicode_failure == 1   ? FVM_EXCEPTION
                                                                      : unicode_failure == 2 ? FVM_ERROR
                                                                                             : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(unicode_getters == 2 && out == (unicode_failure ? 0 : unicode_destination)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->gc_native_depth == 0 && c->script_depth == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (unicode_failure == 1)
            {
                FVM_Value exception;
                if (!(fvm_exception_get(c, &exception) == FVM_OK && exception.number == 81 &&
                      fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
