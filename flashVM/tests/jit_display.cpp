/* Native indexed scene properties and counted target paths match isolated interpreter runs. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/movieclip.h"
#include "flashvm/gc_roots.h"


static FVM_Object *display_setup(FVM_Context *c, FVM_Object **child)
{
    FVM_Object *root = 0;
    FVM_Value args[2], out;
    if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "host-root", 1, &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(args, "child");
    fvm_value_number(args + 1, 2);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *child = out.object;
    if (!(fvm_gc_pin(c, *child) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *holder = 0;
    if (!(fvm_object_create(c, &holder) == FVM_OK && fvm_object_set(c, holder, "native", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    out = class_value(holder);
    if (!(fvm_set_global(c, "clip", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return root;
}


/* GetVariable intentionally hides real MovieClips in SWF4; member reads do not. */
static void display_target(Program &p)
{
    p.variable("clip");
    p.text("native");
    p.op(0x4e);
}
#ifndef FVM_DISPLAY_FIXTURE_ONLY


static FVM_Result forbidden_display(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


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
            FVM_Object *child = 0, *root = display_setup(c, &child);
            FVM_Value out;
            for (unsigned index = 0; index < 2; ++index)
            {
                Program p;
                display_target(p);
                p.number(index);
                p.number(42 + index);
                p.op(0x23);
                display_target(p);
                p.number(index);
                p.op(0x22);
                p.finish();
                out = class_run(c, p, FVM_OK, root);
                if (!(out.type == FVM_VALUE_NUMBER && out.number == 42 + index))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program path_read;
            path_read.text("child");
            path_read.number(0);
            path_read.op(0x22);
            path_read.finish();
            out = class_run(c, path_read, FVM_OK, root);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program selected;
            selected.text("child");
            selected.op(0x20);
            selected.text("");
            selected.number(1);
            selected.op(0x22);
            selected.finish();
            out = class_run(c, selected, FVM_OK, root);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 43))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program missing;
            missing.text("missing");
            missing.number(0);
            missing.op(0x22);
            missing.finish();
            out = class_run(c, missing, FVM_OK, root);
            if (!(out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(c, child, "toString", forbidden_display, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_accessor_rooted(c, child, "_target", forbidden_display, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program path;
            display_target(path);
            path.op(0x45);
            path.finish();
            out = class_run(c, path, FVM_OK, root);
            if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "_level0.child")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value name;
            if (!(fvm_value_string_bytes(&name, "x\0y", 3) == FVM_OK &&
                  fvm_object_set(c, child, "_name", &name) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            out = class_run(c, path, FVM_OK, root);
            if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 11 &&
                  !std::memcmp(out.string, "_level0.x\0y", 11)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_GCRootScope *hold = 0;
            if (!(fvm_gc_roots_begin(c, &hold) == FVM_OK && fvm_gc_roots_add(hold, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !std::memcmp(out.string, "_level0.x\0y", 11)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_roots_end(hold) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program nonclip;
            nonclip.number(42);
            nonclip.op(0x45);
            nonclip.finish();
            out = class_run(c, nonclip, FVM_OK, root);
            if (!(out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned char empty[] = {0x22, 0x23, 0x45};
            for (unsigned i = 0; i < 3; ++i)
            {
                Program p;
                p.op(empty[i]);
                if (i == 1)
                {
                    p.number(42);
                }
                p.finish();
                class_run(c, p, version ? FVM_OK : FVM_ERROR, root);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
