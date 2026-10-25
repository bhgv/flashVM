/* Raster getters observe captured identities, ascending signed depths and deferred replacements. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_order_fixture.hpp"
#include "flashvm/raster.h"
#include "flashvm/property.h"


static void clear_visible(FVM_Context *c, FVM_Object *o)
{
    int removed = 0;
    if (!(fvm_object_delete(c, o, "_visible", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


struct PaintTrace
{
    FVM_Object *root;
    FVM_Object *next;
    FVM_Object *replacement;
    FVM_Object *middle;
    unsigned route;
    unsigned calls;
    unsigned order[8];
};


struct PaintItem
{
    PaintTrace *trace;
    unsigned id;
};


static FVM_Result new_visible(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    PaintTrace *p = static_cast<PaintTrace *>(raw);
    if (!(p->calls < 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p->order[p->calls++] = 4;
    fvm_value_number(out, 1);
    return FVM_OK;
}


static FVM_Result visible(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    PaintItem *item = static_cast<PaintItem *>(raw);
    PaintTrace *p = item->trace;
    if (!(p->calls < 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p->order[p->calls++] = item->id;
    if (item->id == 1 && p->route)
    {
        FVM_Value args[2], result;
        if (p->route == 1)
        {
            p->replacement = ordered_child(c, p->root, "replacement", 2);
            clear_visible(c, p->replacement);
            if (!(fvm_object_define_accessor_rooted(c, p->replacement, "_visible", new_visible, 0, p, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (p->route == 2)
        {
            fvm_value_number(args, -9);
            if (!(fvm_object_call(c, p->next, "swapDepths", 1, args, &result) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (p->route == 3)
        {
            if (!(fvm_object_call(c, p->root, "removeMovieClip", 0, 0, &result) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (p->route == 4)
        {
            return FVM_ERROR;
        }
        else if (p->route == 5 || p->route == 6)
        {
            if (!(fvm_object_call(c, p->route == 5 ? p->next : p->middle, "removeMovieClip", 0, 0, &result) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (!version || version >= 5)
        {
            for (unsigned route = 0; route <= 6; ++route)
            {
                for (unsigned view = 0; view < 2; ++view)
                {
                    FVM_Context *c = tick_context(rt, version);
                    FVM_Object *outer = tick_root(c);
                    FVM_Object *root = ordered_child(c, outer, "paint", 1);
                    PaintTrace trace = {root, 0, 0, 0, route, 0, {0}};
                    FVM_Object *high = ordered_child(c, root, "high", 10);
                    FVM_Object *low = ordered_child(c, root, "low", -5);
                    FVM_Object *middle = ordered_child(c, root, "middle", 2);
                    trace.next = high;
                    trace.middle = middle;
                    PaintItem items[3] = {{&trace, 1}, {&trace, 2}, {&trace, 3}};
                    clear_visible(c, low);
                    clear_visible(c, middle);
                    clear_visible(c, high);
                    if (!(fvm_object_define_accessor_rooted(c, low, "_visible", visible, 0, items, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_define_accessor_rooted(c, middle, "_visible", visible, 0, items + 1, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_define_accessor_rooted(c, high, "_visible", visible, 0, items + 2, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char pixels[16] = {0};
                    FVM_RasterSurface surface = {pixels, 2, 2, 8};
                    FVM_Affine identity = {1, 0, 0, 1, 0, 0};
                    c->avm1_steps = 71;
                    FVM_Result result = view ? fvm_movieclip_render_view(c, root, &surface, &identity)
                                             : fvm_movieclip_render(c, root, &surface);
                    if (!(result == (route == 4 ? FVM_ERROR : FVM_OK)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(trace.calls == ((route == 1 || route >= 5) ? 2U : (route == 3 || route == 4) ? 1U : 3U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(trace.order[0] == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (trace.calls > 1)
                    {
                        if (!(trace.order[1] == ((route == 1 || route == 6) ? 3U : 2U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (trace.calls > 2)
                    {
                        if (!(trace.order[2] == 3))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(c->avm1_steps == 71))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    tick_clean(c);
                    if (route == 1 || route == 2)
                    {
                        trace.route = 0;
                        trace.calls = 0;
                        if (!(fvm_gc_collect(c, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_movieclip_render(c, root, &surface) == FVM_OK && trace.calls == 3))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(trace.order[0] == (route == 2 ? 3U : 1U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(trace.order[1] == (route == 2 ? 1U : 4U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(trace.order[2] == (route == 2 ? 2U : 3U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
