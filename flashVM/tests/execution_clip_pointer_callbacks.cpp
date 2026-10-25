/* Pointer snapshots precede matrix getters; getter exceptions preserve outputs and root scopes. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"

static bool fail_getter, load_in_getter;
static unsigned getter_calls;


static FVM_Result read_x(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (load_in_getter)
    {
        unsigned char swf[] = {'F', 'W', 'S', 4, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        load_in_getter = false;
    }
    ++getter_calls;
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT &&
          fvm_input_set_pointer(c, 100, 200) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 0);
    return fail_getter ? fvm_exception_raise(c, out) : FVM_OK;
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
    for (unsigned version = 3; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        FVM_Object *clip = 0;
        FVM_Value x, y;
        int removed = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK &&
              (version == 3 || fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK) && fvm_standard_install(c) == FVM_OK &&
              fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK &&
              fvm_object_delete(c, clip, "_x", &removed) == FVM_OK && removed &&
              fvm_object_define_accessor_rooted(c, clip, "_x", read_x, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned failing = 0; failing < 2; ++failing)
        {
            fail_getter = failing != 0;
            load_in_getter = version == 3 && !failing;
            getter_calls = 0;
            if (!(fvm_input_set_pointer(c, 5.25, 6.25) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&x, 73);
            fvm_value_number(&y, 74);
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Result r = fvm_exec_clip_pointer(c, clip, &x, &y);
            if (!(getter_calls == 1 && c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (failing)
            {
                if (!(r == FVM_EXCEPTION && x.number == 73 && y.number == 74 && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK && x.number == (version == 3 ? 5.25 : 5) && y.number == (version == 3 ? 6.25 : 6)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            double px, py;
            if (!(fvm_input_get_pointer(c, &px, &py) == FVM_OK && px == 100 && py == 200 &&
                  fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
