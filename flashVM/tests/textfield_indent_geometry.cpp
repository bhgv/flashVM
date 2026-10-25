/* Real faces, alignment, underline, hanging lines, extent and empty fitting share one geometry. */
#define FVM_INSETS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_insets_fixture.hpp"


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
    register_styles(c);
    FVM_Object *field = counted_field(c), *format = inset_format(c);
    inset_number(c, field, "_width", 124);
    inset_bool(c, format, "underline", true);
    const char *align[] = {"left", "center", "right", "justify"};
    const double widths[] = {60, 68, 64, 72};
    for (unsigned face = 0; face < 4; ++face)
    {
        select_style(c, format, face);
        for (unsigned sign = 0; sign < 2; ++sign)
        {
            double indent = sign ? -18 : 18;
            inset_number(c, format, "indent", indent);
            for (unsigned a = 0; a < 4; ++a)
            {
                inset_string(c, format, "align", align[a]);
                inset_apply(c, field, format, "setNewTextFormat");
                inset_string(c, field, "text", "A V");
                double origin = 20 + indent +
                                (a == 1   ? (90 - indent - widths[face]) / 2
                                 : a == 2 ? 90 - indent - widths[face]
                                          : 0);
                FVM_Path *p = 0;
                FVM_PathBounds b;
                int hit = 0;
                if (!(fvm_textfield_path(c, field, &p) == FVM_OK && fvm_path_bounds(p, &b) == FVM_OK &&
                      std::fabs(b.x_min - origin) < 0.02 &&
                      fvm_path_hit_test(p, origin + widths[face] - 1, 42, 0, 1, &hit) == FVM_OK && hit))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(std::fabs(counted_metric(c, field, "textWidth") - (widths[face] + indent)) < 0.02))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_path_destroy(p);
            }
        }
    }
    select_style(c, format, 0);
    inset_number(c, format, "indent", 18);
    inset_string(c, format, "align", "left");
    FVM_Value args[2], out;
    if (!(fvm_value_string(args, "A V A V") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 120);
    if (!(fvm_object_call(c, format, "getTextExtent", 2, args, &out) == FVM_OK &&
          counted_metric(c, out.object, "width") == 90 && counted_metric(c, out.object, "height") == 90 &&
          counted_metric(c, out.object, "textFieldWidth") == 124))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_apply(c, field, format, "setNewTextFormat");
    inset_number(c, field, "_x", 100);
    inset_string(c, field, "text", "AV");
    inset_string(c, field, "autoSize", "center");
    if (!(counted_metric(c, field, "_width") == 100 && counted_metric(c, field, "_x") == 112))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_string(c, field, "text", "");
    if (!(counted_metric(c, field, "_width") == 34 && counted_metric(c, field, "_x") == 145))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
