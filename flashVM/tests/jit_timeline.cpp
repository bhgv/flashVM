/* Native timeline commands preserve intrinsic dispatch, frame policy and selected target scope. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/movieclip.h"


static FVM_Result timeline_forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static FVM_Object *timeline_setup(FVM_Context *c, FVM_Object **child = 0)
{
    FVM_Object *root = 0;
    FVM_Value args[2], out;
    const unsigned char empty[] = {0};
    if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 4, &root) == FVM_OK &&
          fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_define_frame(c, root, 2, "two", empty, sizeof(empty)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *names[] = {"nextFrame", "prevFrame", "play", "stop", "gotoAndPlay", "gotoAndStop"};
    for (unsigned i = 0; i < 6; ++i)
    {
        if (!(fvm_object_define_method_rooted(c, root, names[i], timeline_forbidden, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (child)
    {
        fvm_value_string(args, "child");
        fvm_value_number(args + 1, 2);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        *child = out.object;
    }
    return root;
}


static void timeline_state(FVM_Object *clip, unsigned frame, bool playing)
{
    FVM_Object *data = clip->resource_result.object;
    if (!(data->properties["frame"].number == frame && data->properties["playing"].number == (playing ? 1 : 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void timeline_direct(Program &p, unsigned index)
{
    p.op(0x81);
    p.op(2);
    p.op(0);
    p.op((unsigned char)index);
    p.op((unsigned char)(index >> 8));
}


static void timeline_label(Program &p, const char *label)
{
    size_t n = std::strlen(label) + 1;
    p.op(0x8c);
    p.op((unsigned char)n);
    p.op((unsigned char)(n >> 8));
    for (size_t i = 0; i < n; ++i)
    {
        p.op((unsigned char)label[i]);
    }
}


static void timeline_dynamic(Program &p, unsigned flags, unsigned bias = 0)
{
    p.op(0x9f);
    p.op((flags & 2) ? 3 : 1);
    p.op(0);
    p.op((unsigned char)flags);
    if (flags & 2)
    {
        p.op((unsigned char)bias);
        p.op((unsigned char)(bias >> 8));
    }
}


static void timeline_run(FVM_Context *c, FVM_Object *root, Program &p, unsigned frame, bool playing)
{
    p.finish();
    FVM_Value out = class_run(c, p, FVM_OK, root);
    if (!(out.type == FVM_VALUE_NUMBER && out.number == 31))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    timeline_state(root, frame, playing);
}
#ifndef FVM_TIMELINE_FIXTURE_ONLY


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            if (version && version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Object *child = 0, *root = timeline_setup(c, &child);
            const unsigned char ops[] = {0x05, 0x04, 0x06, 0x07};
            const unsigned frames[] = {1, 2, 2, 2};
            for (unsigned i = 0; i < 4; ++i)
            {
                Program p;
                p.number(31);
                p.op(ops[i]);
                timeline_run(c, root, p, frames[i], i == 2);
            }
            Program direct;
            direct.number(31);
            timeline_direct(direct, 3);
            timeline_run(c, root, direct, 4, false);
            Program zero;
            zero.number(31);
            timeline_direct(zero, 0);
            timeline_run(c, root, zero, 1, false);
            Program last;
            last.number(31);
            timeline_direct(last, 65535);
            timeline_run(c, root, last, 4, false);
            Program clamp;
            clamp.number(31);
            clamp.op(0x04);
            timeline_run(c, root, clamp, 4, false);
            Program label;
            label.number(31);
            timeline_label(label, "two");
            timeline_run(c, root, label, 2, false);
            Program absent;
            absent.number(31);
            timeline_label(absent, "absent");
            timeline_run(c, root, absent, 2, false);
            Program dynamic;
            dynamic.number(31);
            dynamic.number(1);
            timeline_dynamic(dynamic, 3, 2);
            timeline_run(c, root, dynamic, 3, true);
            Program decimal;
            decimal.number(31);
            decimal.text("002");
            timeline_dynamic(decimal, 0);
            timeline_run(c, root, decimal, 2, false);
            Program biased_label;
            biased_label.number(31);
            biased_label.text("two");
            timeline_dynamic(biased_label, 2, 65535);
            timeline_run(c, root, biased_label, 2, false);
            Program maximum;
            maximum.number(31);
            maximum.number(1);
            timeline_dynamic(maximum, 3, 65535);
            timeline_run(c, root, maximum, 4, true);
            Program negative;
            negative.number(31);
            negative.number((unsigned)-1);
            timeline_dynamic(negative, 0);
            timeline_run(c, root, negative, 1, false);
            Program selected;
            selected.number(31);
            selected.text("child");
            selected.op(0x20);
            selected.op(0x07);
            timeline_run(c, root, selected, 1, false);
            timeline_state(child, 1, false);
            Program missing;
            missing.number(31);
            missing.text("missing");
            missing.op(0x20);
            missing.op(0x06);
            timeline_run(c, root, missing, 1, false);
            Program recover;
            recover.number(31);
            recover.text("missing");
            recover.op(0x20);
            recover.number(4);
            timeline_dynamic(recover, 0);
            timeline_run(c, root, recover, 4, false);
            Program empty;
            timeline_dynamic(empty, 0);
            empty.number(31);
            empty.finish();
            class_run(c, empty, version ? FVM_OK : FVM_ERROR, root);
            fvm_context_destroy(c);
        }
    }
    /* A native context with no timeline propagates the same unsupported status. */
    for (class_mode = 0; class_mode < 3; ++class_mode)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program p;
        p.op(0x06);
        p.finish();
        class_run(c, p, FVM_UNSUPPORTED);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
