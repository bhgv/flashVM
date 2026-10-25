/* One-based visual lines, pixel overflow and exact contour translations from analytic font metrics. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_fixture.hpp"
#include <limits>


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    FVM_TextFieldScrollInfo i = scroll_info(c, field);
    if (!(i.scroll == 1 && !i.hscroll && i.maxscroll == 2 && i.maxhscroll == 12 && i.bottom_scroll == 2 &&
          i.line_count == 3 && i.line_height == 46 && i.clip_x == 2 && i.clip_y == 2 && i.clip_width == 36 &&
          i.clip_height == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double width = counted_metric(c, field, "textWidth"), height = counted_metric(c, field, "textHeight");
    FVM_PathBounds initial = scroll_bounds(c, field);
    scroll_number(c, field, "scroll", 2.9);
    scroll_number(c, field, "hscroll", 7.9);
    i = scroll_info(c, field);
    if (!(i.scroll == 2 && i.hscroll == 7 && i.bottom_scroll == 3 && i.offset_x == -7 && i.offset_y == -46))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_PathBounds moved = scroll_bounds(c, field);
    if (!(moved.x_min == initial.x_min - 7 && moved.x_max == initial.x_max - 7 && moved.y_min == initial.y_min - 46 &&
          moved.y_max == initial.y_max - 46))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(counted_metric(c, field, "textWidth") == width && counted_metric(c, field, "textHeight") == height &&
          counted_metric(c, field, "maxscroll") == 2 && counted_metric(c, field, "maxhscroll") == 12 &&
          counted_metric(c, field, "bottomScroll") == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned k = 0; k < 2; ++k)
    {
        scroll_number(c, field, k ? "hscroll" : "scroll", 1e100);
        if (!(counted_metric(c, field, k ? "hscroll" : "scroll") == (k ? 12 : 2)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scroll_number(c, field, k ? "hscroll" : "scroll", -1e100);
        if (!(counted_metric(c, field, k ? "hscroll" : "scroll") == (k ? 0 : 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value v;
    double invalid[] = {std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::quiet_NaN()};
    for (unsigned k = 0; k < 3; ++k)
    {
        fvm_value_number(&v, invalid[k]);
        if (!(fvm_object_set(c, field, "scroll", &v) == FVM_INVALID_ARGUMENT &&
              counted_metric(c, field, "scroll") == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_TextFieldScrollInfo saved = i;
    --i.struct_size;
    saved = i;
    if (!(fvm_textfield_scroll_info(c, field, &i) == FVM_INVALID_ARGUMENT && !std::memcmp(&i, &saved, sizeof(i))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_textfield_scroll_info(0, field, &i) == FVM_INVALID_ARGUMENT && !std::memcmp(&i, &saved, sizeof(i))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, field, "_height", 0);
    i = scroll_info(c, field);
    if (!(i.maxscroll == 3 && i.bottom_scroll == 1 && !i.clip_height))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, field, "_height", 200);
    i = scroll_info(c, field);
    if (!(i.maxscroll == 1 && i.bottom_scroll == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, field, "_height", 94);
    scroll_number(c, field, "scroll", 2);
    scroll_number(c, field, "hscroll", 12);
    if (!(fvm_value_string(&v, "A") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    i = scroll_info(c, field);
    if (!(i.scroll == 1 && !i.hscroll && i.maxscroll == 1 && !i.maxhscroll))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#else
    FVM_TextFieldScrollInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_textfield_scroll_info(0, 0, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
