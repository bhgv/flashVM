/* Invalid raster child slots and private edges fail safely; repairing the graph restores use. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_order_fixture.hpp"
#include "flashvm/raster.h"
#include "flashvm/property.h"

static unsigned child_calls;


static FVM_Result observe(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++child_calls;
    fvm_value_number(out, 1);
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
    for (unsigned mode = 0; mode < 14; ++mode)
    {
        FVM_Context *c = tick_context(rt), *other = tick_context(rt);
        FVM_Object *root = tick_root(c);
        FVM_Object *child = ordered_child(c, root, "child", 1), *foreign = tick_root(other), *plain = 0;
        if (!(fvm_object_create(c, &plain) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *data = root->resource_result.object, *children = 0;
        if (!(fvm_movieclip_storage::private_object(data, "children", children) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int deleted = 0;
        if (!(fvm_object_delete(c, child, "_visible", &deleted) == FVM_OK && deleted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, child, "_visible", observe, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        child_calls = 0;
        FVM_Value bad = tick_object(0), saved;
        if (mode == 0)
        {
            fvm_value_number(&bad, 17);
        }
        if (mode == 1)
        {
            bad = tick_object(foreign);
        }
        if (mode == 2)
        {
            bad = tick_object(child);
        }
        if (mode == 3)
        {
            bad = tick_object(root);
        }
        if (mode == 4)
        {
            bad = tick_object(plain);
        }
        if (mode < 5)
        {
            children->indexed[999] = bad;
        }
        else if (mode < 9)
        {
            const char *field = mode < 7 ? "children" : "paths";
            saved = data->properties[field];
            data->properties[field] = tick_object(mode % 2 ? 0 : foreign);
        }
        else if (mode == 9)
        {
            for (unsigned long i = 0; i <= FVM_MOVIECLIP_RENDER_MAX_CHILDREN; ++i)
            {
                children->indexed[i] = tick_object(child);
            }
        }
        else
        {
            FVM_Object *selected = mode < 12 ? root : child;
            saved = selected->resource_result;
            selected->resource_result = tick_object(mode % 2 ? foreign->resource_result.object : 0);
        }
        unsigned char pixels[16] = {0};
        FVM_RasterSurface surface = {pixels, 2, 2, 8};
        if (!(fvm_movieclip_render(c, root, &surface) == FVM_INVALID_ARGUMENT && !child_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        if (mode < 5)
        {
            children->indexed.erase(999);
        }
        else if (mode < 9)
        {
            data->properties[mode < 7 ? "children" : "paths"] = saved;
        }
        else if (mode == 9)
        {
            children->indexed.clear();
            children->indexed[FVM_MOVIECLIP_CHILD_SLOT_BIAS + 1] = tick_object(child);
        }
        else
        {
            (mode < 12 ? root : child)->resource_result = saved;
        }
        if (!(fvm_movieclip_render(c, root, &surface) == FVM_OK && child_calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_context_destroy(other);
    }
    FVM_Context *c = tick_context(rt);
    FVM_Object *root = tick_root(c), *leaf = root;
    for (unsigned i = 0; i < FVM_MOVIECLIP_RENDER_MAX_DEPTH; ++i)
    {
        leaf = ordered_child(c, leaf, "nested", 1);
    }
    unsigned char pixels[16] = {0};
    FVM_RasterSurface surface = {pixels, 2, 2, 8};
    if (!(fvm_movieclip_render(c, root, &surface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ordered_child(c, leaf, "tooDeep", 1);
    if (!(fvm_movieclip_render(c, root, &surface) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    FVM_Affine view = {1, 0, 0, 1, 0, 0};
    if (!(fvm_movieclip_render_view(c, root, &surface, &view) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
