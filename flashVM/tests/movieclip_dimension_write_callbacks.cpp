/* Value coercion may detach and collect before atomic geometry publication. */
#define FVM_DIMENSION_WRITE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_dimension_write.cpp"
#include "flashvm/function.h"
#include "flashvm/exception.h"
#include "flashvm/property.h"

static FVM_Object *selected;
static unsigned failure;


static FVM_Result dimension_value(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    if (!(!c->gc_native_depth && fvm_exec_detach_clip(c, selected) == FVM_OK))
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
    if (!(fvm_gc_collect(c, &reclaimed) == FVM_OK && reclaimed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 1)
    {
        fvm_value_number(out, 81);
        return fvm_exception_raise(c, out);
    }
    if (failure == 2)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 40);
    return FVM_OK;
}

static FVM_Object *geometry_owner;
static bool geometry_fail;


static FVM_Result dimension_transform(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, geometry_owner, "sideEffect", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (geometry_fail)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 0);
    return FVM_OK;
}


static FVM_Result dimension_forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
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
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (failure = 0; failure < 3; ++failure)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            dimension_load(c, version);
            FVM_Object *root = 0, *input = 0, *fn = 0;
            if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            selected = child(c, root, "clip", 1);
            size_rect(c, selected, 0, 0, 10, 20);
            const unsigned char code[] = {0};
            if (!(fvm_construct(c, "Object", 0, 0, &input) == FVM_OK &&
                  fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
                  fvm_function_set_body(c, fn, dimension_value, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value = size_object(fn);
            if (!(fvm_object_set(c, input, "valueOf", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value target = size_object(selected), at;
            value = size_object(input);
            fvm_value_number(&at, 8);
            FVM_Result r = fvm_exec_display_property(c, 1, &target, &at, &value, &target);
            if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && fvm_gc_pin(c, selected) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!failure)
            {
                if (!(r == FVM_OK && target.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                dimension_scale(c, selected, 400, 100);
            }
            else
            {
                if (!(r == (failure == 1 ? FVM_EXCEPTION : FVM_ERROR) && target.object == selected))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                dimension_scale(c, selected, 100, 100);
                if (failure == 1)
                {
                    if (!(fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            dimension_write(c, selected, 9, 40);
            size_expect(c, selected, 9, 40);
            fvm_context_destroy(c);
        }
    }
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dimension_load(c, 8);
    if (!(fvm_movieclip_create(c, "root", 1, &geometry_owner) == FVM_OK && fvm_gc_pin(c, geometry_owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_rect(c, geometry_owner, 0, 0, 10, 20);
    FVM_Object *nested = child(c, geometry_owner, "nested", 1);
    size_rect(c, nested, 0, 0, 5, 5);
    int deleted = 0;
    if (!(fvm_object_delete(c, nested, "_x", &deleted) == FVM_OK && deleted &&
          fvm_object_define_accessor_rooted(c, nested, "_x", dimension_transform, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, geometry_owner, "_x", &deleted) == FVM_OK && deleted &&
          fvm_object_define_accessor_rooted(c, geometry_owner, "_x", dimension_forbidden, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, geometry_owner, "_width", 0, dimension_forbidden, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value target = size_object(geometry_owner), at, value, out;
    fvm_value_number(&at, 8);
    fvm_value_number(&value, 40);
    fvm_value_number(&out, 77);
    geometry_fail = true;
    if (!(fvm_exec_display_property(c, 1, &target, &at, &value, &out) == FVM_ERROR && out.number == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dimension_scale(c, geometry_owner, 100, 100);
    if (!(fvm_object_get(c, geometry_owner, "sideEffect", &out) == FVM_OK && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry_fail = false;
    dimension_write(c, geometry_owner, 8, 40);
    dimension_scale(c, geometry_owner, 400, 100);
    if (!(!c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
