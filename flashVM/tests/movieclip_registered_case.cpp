/* Attach/duplicate select version-folded classes, including transported fallback registries. */
#define main sprite_case_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_resources.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "../src/core/runtime.hpp"

static unsigned constructed;


static FVM_Result case_construct(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++constructed;
    FVM_Value value;
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, frame->receiver, "marker", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
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
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes empty, controls, tags;
        tag(controls, 1, empty);
        tag(controls, 0, empty);
        tag(tags, 39, sprite(7, 1, controls));
        export_asset(tags, 7, "Hero");
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        Bytes swf = movie(tags, 1);
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value ctor = registered_constructor(c), klass, args[2], out;
        if (!(fvm_function_set_body(c, ctor.object, case_construct, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "hERO");
        args[1] = ctor;
        if (!(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        constructed = 0;
        FVM_Object *clip = attach(c, root, "Hero", "clip", 1);
        if (!(clip && constructed == (version < 7 ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version < 7)
        {
            number(c, clip, "marker", 17);
        }
        FVM_Object *copy = 0;
        FVM_Value initial, name, depth;
        fvm_value_undefined(&initial);
        fvm_value_string(&name, "copy");
        fvm_value_number(&depth, 3);
        if (!(fvm_exec_duplicate_clip(c, clip, &name, &depth, &initial, &out) == FVM_OK &&
              out.type == FVM_VALUE_OBJECT && constructed == (version < 7 ? 2U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        copy = out.object;
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version < 7)
        {
            number(c, copy, "marker", 17);
        }
        FVM_Context *other = 0;
        FVM_Channel *ch = 0;
        FVM_Value root_value;
        if (!(fvm_context_create(rt, &other) == FVM_OK && fvm_load_memory(other, swf.begin(), swf.size()) == FVM_OK &&
              fvm_channel_create(&ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&root_value);
        root_value.type = FVM_VALUE_OBJECT;
        root_value.object = root;
        if (!(fvm_channel_send_value(c, ch, &root_value, 0) == FVM_OK &&
              fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *transported = out.object;
        if (!(fvm_gc_pin(other, transported) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *attached = attach(other, transported, "Hero", "transported", 5);
        if (!(attached))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value parent = get(other, attached, "createdParent");
        if (version < 7)
        {
            if (!(parent.type == FVM_VALUE_OBJECT && parent.object == transported))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(parent.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !other->gc_frames && !other->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
