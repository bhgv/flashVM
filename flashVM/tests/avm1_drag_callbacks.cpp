/* StartDrag roots a resolved source through collecting flags/limits and ordered exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "flashvm/input.h"
#include "../src/core/runtime.hpp"

static unsigned seen[7], seen_count, throw_phase;
static FVM_Object *source_clip;
static const char *keys[] = {
    "sourceInput", "lockInput", "constraintInput", "ymaxInput", "xmaxInput", "yminInput", "xminInput"};


static FVM_Result convert(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    if (!(seen_count < 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    seen[seen_count++] = id;
    FVM_Value value;
    fvm_value_undefined(&value);
    for (unsigned i = 0; i < 7; ++i)
    {
        if (!(fvm_set_global(c, keys[i], &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (id == 2)
    {
        if (!(fvm_exec_detach_clip(c, source_clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, frame->receiver, "alive", &value) == FVM_OK &&
          value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (throw_phase == id)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (id == 1)
    {
        return fvm_value_string(out, "clip");
    }
    fvm_value_number(out, id <= 3 ? 1 : id <= 5 ? 10 : 0);
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned fail = 0; fail < 8; ++fail)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0, *input;
            FVM_Value a[2], out, value;
            unsigned ids[] = {1, 2, 3, 4, 5, 6, 7};
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(a, "clip");
            fvm_value_number(a + 1, 1);
            if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            source_clip = out.object;
            fvm_value_number(&value, 17);
            if (!(fvm_object_set(c, source_clip, "alive", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 7; ++i)
            {
                if (!(fvm_object_create(c, &input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set(c, input, "alive", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                attach(c, input, i ? "valueOf" : "toString", convert, ids + i);
                fvm_value_undefined(&out);
                out.type = FVM_VALUE_OBJECT;
                out.object = input;
                if (!(fvm_set_global(c, keys[i], &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Bytes b;
            for (unsigned i = 7; i > 0; --i)
            {
                text(b, keys[i - 1]);
                b.push_back(0x1c);
            }
            b.push_back(0x27);
            b.push_back(0);
            seen_count = 0;
            throw_phase = fail;
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Result r = fvm_avm1_execute_receiver(c, b.begin(), b.size(), root);
            if (!(seen_count == (fail ? fail : 7) && c->gc_frames == roots && !c->avm1_target_active &&
                  !c->avm1_execution_depth && !c->gc_native_depth && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < seen_count; ++i)
            {
                if (!(seen[i] == i + 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (fail)
            {
                if (!(r == FVM_EXCEPTION && !c->drag_clip && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK && c->drag_clip == source_clip &&
                      fvm_object_get(c, source_clip, "alive", &out) == FVM_OK && out.number == 17 &&
                      fvm_exec_drag_update(c) == FVM_OK && !c->drag_clip))
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
    return 0;
}
