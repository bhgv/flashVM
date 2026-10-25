/* Raw GetProperty reads context pointer in selected clip space; legacy readonly writes skip coercion. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "flashvm/input.h"
#include "../src/core/runtime.hpp"

static unsigned conversions;


static FVM_Result unexpected_number(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++conversions;
    fvm_value_number(out, 91);
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
        FVM_Context *c = 0;
        FVM_Object *root = 0, *input = 0;
        FVM_Value a[2], out, value;
        FVM_Affine transform = {1, 0, 0, 1, 10, 20};
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_set_matrix(c, root, &transform) == FVM_OK))
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
        FVM_Object *clip = out.object;
        fvm_value_number(&value, 5);
        if (!(fvm_object_set(c, clip, "_x", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 7);
        if (!(fvm_object_set(c, clip, "_y", &value) == FVM_OK && fvm_input_set_pointer(c, 18, 31) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        attach(c, input, "valueOf", unexpected_number, 0);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = input;
        if (!(fvm_set_global(c, "input", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes b;
        text(b, "mouseX");
        text(b, "clip");
        integer(b, 20);
        b.push_back(0x22);
        b.push_back(0x1d);
        text(b, "mouseY");
        text(b, "clip");
        integer(b, 21);
        b.push_back(0x22);
        b.push_back(0x1d);
        for (unsigned index = 20; index <= 21; ++index)
        {
            text(b, "clip");
            integer(b, index);
            text(b, "input");
            b.push_back(0x1c);
            b.push_back(0x23);
        }
        b.push_back(0);
        conversions = 0;
        FVM_GCRootFrame *roots = c->gc_frames;
        if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), root) == FVM_OK && !conversions &&
              c->gc_frames == roots && !c->avm1_target_active && !c->avm1_execution_depth && !c->gc_native_depth &&
              !c->script_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, root, "mouseX", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 3 &&
              fvm_object_get(c, root, "mouseY", &out) == FVM_OK && out.number == 4 && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
