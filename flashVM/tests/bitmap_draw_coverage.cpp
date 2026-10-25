#define FVM_BITMAP_DRAW_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "bitmap_draw.cpp"


static inline unsigned alpha_at(FVM_Context *c, FVM_Object *image, unsigned x, unsigned y = 0)
{
    return (unsigned)((unsigned long)read(c, image, x, y) >> 24);
}


static FVM_Result counted_position(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{
    ++*(unsigned *)u;
    fvm_value_number(out, 2);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *source = bitmap(c, 2, 1, true, 0xffff0000UL);
    write(c, source, 1, 0, 0x0000ff00UL);
    FVM_Object *masked = bitmap(c, 4, 1, true, 0xc8123456UL);
    FVM_Result rendered = draw_result(c, masked, source, 0, 0, 11);
    if (!(rendered == FVM_OK || rendered == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (rendered == FVM_OK)
    {
        if (!(read(c, masked, 0, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, masked, 1, 0) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, masked, 2, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, masked, 3, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *erased = bitmap(c, 4, 1, true, 0xc8123456UL);
    if (draw_result(c, erased, source, 0, 0, 12) == FVM_OK)
    {
        if (!(read(c, erased, 0, 0) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, erased, 1, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, erased, 2, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *partial = bitmap(c, 1, 1, true, 0x80010203UL);
    FVM_Object *half = bitmap(c, 1, 1, true, 0xc8123456UL);
    if (draw_result(c, half, partial, 0, 0, 11) == FVM_OK)
    {
        if (!(alpha_at(c, half, 0) == 100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(((unsigned long)read(c, half, 0, 0) & 0xffffffUL) == 0x123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    half = bitmap(c, 1, 1, true, 0xc8123456UL);
    if (draw_result(c, half, partial, 0, 0, 12) == FVM_OK)
    {
        if (!(alpha_at(c, half, 0) == 100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(((unsigned long)read(c, half, 0, 0) & 0xffffffUL) == 0x123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *self = bitmap(c, 1, 1, true, 0x80123456UL);
    if (draw_result(c, self, self, 0, 0, 11) == FVM_OK)
    {
        if (!(alpha_at(c, self, 0) == 64))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(((unsigned long)read(c, self, 0, 0) & 0xffffffUL) == 0x123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    double moved_values[] = {1, 0, 0, 1, .5, 0};
    FVM_Object *moved = make(c, "flash.geom.Matrix", 6, moved_values), *zero = bitmap(c, 1, 1, true, 0);
    FVM_Object *edge = bitmap(c, 3, 1, true, 0xc8123456UL);
    if (draw_result(c, edge, zero, moved, 0, 11) == FVM_OK)
    {
        if (!(alpha_at(c, edge, 0) >= 99 && alpha_at(c, edge, 0) <= 101))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(alpha_at(c, edge, 1) >= 99 && alpha_at(c, edge, 1) <= 101))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(alpha_at(c, edge, 2) == 200))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    edge = bitmap(c, 3, 1, true, 0xc8123456UL);
    if (draw_result(c, edge, partial, moved, 0, 11) == FVM_OK)
    {
        if (!(alpha_at(c, edge, 0) >= 149 && alpha_at(c, edge, 0) <= 151))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(alpha_at(c, edge, 1) >= 149 && alpha_at(c, edge, 1) <= 151))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(alpha_at(c, edge, 2) == 200))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_create(c, "transparent", 1, &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value fill[2];
    fvm_value_number(fill, 0xff0000);
    fvm_value_number(fill + 1, 0);
    call(c, clip, "beginFill", 2, fill);
    geometry(c, clip, true);
    FVM_Object *target = bitmap(c, 12, 4, true, 0xc8123456UL);
    if (draw_result(c, target, clip, 0, 0, 11) == FVM_OK)
    {
        if (!(read(c, target, 0, 0) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 7, 3) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 8, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 11, 3) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *empty = 0;
    if (!(fvm_movieclip_create(c, "empty", 1, &empty) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, empty) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *unchanged = bitmap(c, 4, 1, true, 0xc8123456UL);
    if (!(draw_result(c, unchanged, empty, 0, 0, 11) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read(c, unchanged, 0, 0) == 0xc8123456UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double clipping_values[] = {1, 0, 1, 1};
    FVM_Object *clipping = rectangle(c, clipping_values);
    target = bitmap(c, 4, 1, true, 0xc8123456UL);
    if (draw_result(c, target, source, 0, 0, 11, clipping) == FVM_OK)
    {
        if (!(read(c, target, 0, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 1, 0) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 2, 0) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *hole = 0;
    if (!(fvm_movieclip_create(c, "hole", 1, &hole) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, hole) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, hole, "beginFill", 2, fill);
    geometry(c, hole, false);
    const double inner[] = {2, 2, 2, 6, 6, 6, 6, 2, 2, 2};
    for (unsigned i = 0; i < 10; i += 2)
    {
        FVM_Value a[2];
        fvm_value_number(a, inner[i]);
        fvm_value_number(a + 1, inner[i + 1]);
        call(c, hole, i ? "lineTo" : "moveTo", 2, a);
    }
    call(c, hole, "endFill");
    target = bitmap(c, 8, 8, true, 0xc8123456UL);
    if (draw_result(c, target, hole, 0, 0, 11) == FVM_OK)
    {
        if (!(read(c, target, 1, 1) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 4, 4) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *stroke = 0;
    if (!(fvm_movieclip_create(c, "stroke", 1, &stroke) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, stroke) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value line[3];
    fvm_value_number(line, 2);
    fvm_value_number(line + 1, 0xff0000);
    fvm_value_number(line + 2, 0);
    call(c, stroke, "lineStyle", 3, line);
    FVM_Value point[2];
    fvm_value_number(point, 0);
    fvm_value_number(point + 1, 0);
    call(c, stroke, "moveTo", 2, point);
    fvm_value_number(point, 8);
    call(c, stroke, "lineTo", 2, point);
    target = bitmap(c, 12, 4, true, 0xc8123456UL);
    if (draw_result(c, target, stroke, 0, 0, 11) == FVM_OK)
    {
        if (!(read(c, target, 3, 0) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(read(c, target, 3, 2) == 0xc8123456UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *parent = 0;
    if (!(fvm_movieclip_create(c, "counter", 1, &parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *nested = child(c, parent, "nested", 1);
    fill_color(c, nested, 0xff0000);
    int removed = 0;
    if (!(fvm_object_delete(c, nested, "_x", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned accesses = 0;
    if (!(fvm_object_define_accessor_rooted(c, nested, "_x", counted_position, 0, &accesses, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    target = bitmap(c, 12, 4, true, 0xc8123456UL);
    FVM_Result counted = draw_result(c, target, parent, 0, 0, 11);
    if (counted == FVM_OK)
    {
        if (!(accesses == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(counted == FVM_UNSUPPORTED))
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
    if (rendered == FVM_OK)
    {
        if (!(draw_result(c, masked, source, 0, 0, 11) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
