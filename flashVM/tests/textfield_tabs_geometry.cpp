/* All alignments, real faces, fractional tracking, margin translation and underline agree. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_tabs_fixture.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c);
    register_styles(c);
    double stops[] = {40, 80};
    FVM_Object *format = tabs_format(c, stops, 2);
    inset_number(c, format, "leftMargin", 10);
    inset_number(c, format, "rightMargin", 12);
    inset_number(c, format, "blockIndent", 8);
    inset_number(c, format, "letterSpacing", 0.5);
    inset_bool(c, format, "underline", true);
    inset_number(c, field, "_width", 124);
    const char *align[] = {"left", "center", "right"};
    const double advances[] = {64.5, 68.5, 66.5, 70.5};
    for (unsigned face = 0; face < 4; ++face)
    {
        select_style(c, format, face);
        for (unsigned a = 0; a < 3; ++a)
        {
            inset_string(c, format, "align", align[a]);
            inset_apply(c, field, format, "setNewTextFormat");
            inset_string(c, field, "text", "A\tV");
            double shift = a == 1 ? (90 - advances[face]) / 2 : a == 2 ? 90 - advances[face] : 0;
            FVM_Path *p = 0;
            FVM_PathBounds b;
            int hit = 0;
            if (!(counted_metric(c, field, "textWidth") == advances[face] &&
                  fvm_textfield_path(c, field, &p) == FVM_OK && fvm_path_bounds(p, &b) == FVM_OK &&
                  std::fabs(b.x_min - 20 - shift) < 1e-9 &&
                  fvm_path_hit_test(p, 55 + shift, 42, 0, 1, &hit) == FVM_OK && hit))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_path_destroy(p);
        }
    }
    select_style(c, format, 0);
    inset_string(c, format, "align", "left");
    inset_number(c, format, "letterSpacing", 0);
    inset_number(c, format, "indent", 10);
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "A\tV");
    inset_string(c, field, "autoSize", "left");
    if (!(counted_metric(c, field, "_width") == 98))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_number(c, format, "indent", 0);
    inset_apply(c, field, format, "setTextFormat");
    inset_bool(c, field, "multiline", true);
    inset_bool(c, field, "wordWrap", true);
    inset_number(c, field, "_width", 74);
    if (!(counted_metric(c, field, "textWidth") == 40 && counted_metric(c, field, "_height") == 94))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
