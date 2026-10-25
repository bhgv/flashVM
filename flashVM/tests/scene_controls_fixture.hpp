/* Native constructor fixtures for captured authored scene controls. */
#define FVM_SCENE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_scene.cpp"
#include "movieclip_tick_fixture.hpp"
#include "../src/standard/movieclip_constants.h"
#include "flashvm/jit.h"
#include "flashvm/execution.h"


struct SceneControlProbe
{
    FVM_Object *list;
    FVM_Object *future;
    FVM_Object *replacement;
    unsigned calls;
    unsigned mutation;
    long depths[8];
};


static FVM_Result
scene_control_constructor(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    SceneControlProbe *p = static_cast<SceneControlProbe *>(raw);
    FVM_Value depth;
    FVM_Result result = fvm_object_call(c, self, "getDepth", 0, 0, &depth);
    if (result != FVM_OK)
    {
        return result;
    }
    if (!(p->calls < 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p->depths[p->calls++] = (long)depth.number;
    if (p->calls == 1)
    {
        if (p->mutation == 1)
        {
            p->list->indexed[1] = tick_object(p->replacement);
        }
        if (p->mutation == 2)
        {
            FVM_Value value;
            fvm_value_number(&value, 4);
            p->future->properties["depth"] = value;
        }
        if (p->mutation == 3)
        {
            p->list->indexed.clear();
        }
        if (p->mutation == 4)
        {
            p->list->indexed[3] = tick_object(p->replacement);
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Object *scene_control_root(FVM_Context *c, SceneControlProbe &p, unsigned version = 8, bool bind = true)
{
    Bytes tags, empty, exports;
    definition(tags);
    u16(exports, 1);
    u16(exports, 7);
    text(exports, "Hero");
    tag(tags, 56, exports);
    tag(tags, 26, place(2 | 4 | 32, 1, 7, 0, 0, "first"));
    tag(tags, 26, place(2 | 4 | 32, 2, 7, 0, 0, "second"));
    tag(tags, 26, place(2 | 4 | 32, 3, 7, 0, 0, "third"));
    tag(tags, 1, empty);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 2);
    swf[3] = (unsigned char)version;
    FVM_Object *root = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *frames = root->resource_result.object->properties["frames"].object;
    p.list = frames->indexed[1].object->properties["controls"].object;
    p.future = p.list->indexed[1].object;
    p.calls = 0;
    p.mutation = 0;
    if (!(fvm_object_create(c, &p.replacement) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p.replacement->properties = p.future->properties;
    FVM_Value v;
    fvm_value_number(&v, 4);
    p.replacement->properties["depth"] = v;
    if (!(fvm_value_string(&v, "late") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, p.replacement, "name", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, p.replacement) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!bind)
    {
        return root;
    }
    FVM_Object *fn = 0;
    if (!(fvm_function_create_native(c, scene_control_constructor, &p, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value mc;
    if (!(fvm_get_global(c, "MovieClip", &mc) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = get(c, mc.object, "prototype");
    if (!(fvm_object_set(c, fn, "prototype", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value klass;
    if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[2], out;
    if (!(fvm_value_string(args, "Hero") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    args[1] = tick_object(fn);
    if (!(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return root;
}
