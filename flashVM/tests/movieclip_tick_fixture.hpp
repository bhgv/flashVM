/* Shared public MovieClip creation and receiver-state assertions for tick regressions. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/stage.h"


static FVM_Value tick_object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}


static FVM_Context *tick_context(FVM_Runtime *rt, unsigned version = 8)
{
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (version)
    {
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
    return c;
}


static FVM_Object *tick_root(FVM_Context *c, unsigned frames = 1)
{
    FVM_Object *o = 0;
    if (!(fvm_movieclip_create(c, "root", frames, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return o;
}


static FVM_Object *tick_child(FVM_Context *c, FVM_Object *root, const char *name, unsigned depth)
{
    FVM_Value a[2], out;
    if (!(fvm_value_string(a, name) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(a + 1, depth);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}


static void tick_clean(FVM_Context *c)
{
    if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth && !c->calling_owner &&
          !c->calling_function))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


struct TickProbe
{
    unsigned calls;
    FVM_Result status;


    TickProbe() : calls(0), status(FVM_OK)
    {
    }
};


static FVM_Result tick_notify(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *raw)
{
    TickProbe *p = (TickProbe *)raw;
    if (!(n == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->calls;
    fvm_value_undefined(out);
    if (p->status == FVM_EXCEPTION)
    {
        FVM_Value v;
        fvm_value_number(&v, 19);
        return fvm_exception_raise(c, &v);
    }
    return p->status;
}
