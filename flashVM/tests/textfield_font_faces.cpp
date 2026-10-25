/* Original distinct faces prove source selection for metrics, contours and independent kerning. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"
#include "fixtures/font_bold_data.h"
#include "fixtures/font_italic_data.h"
#include "fixtures/font_bold_italic_data.h"
#include <cmath>

#ifdef FVM_HAS_FONTS


static void register_styles(FVM_Context *c)
{
    if (!(fvm_standard_text_register_font_variant(
              c, "Geometry", FVM_TEXT_FONT_BOLD, fvm_test_font_bold, sizeof(fvm_test_font_bold), 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_text_register_font_variant(
              c, "Geometry", FVM_TEXT_FONT_ITALIC, fvm_test_font_italic, sizeof(fvm_test_font_italic), 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_text_register_font_variant(
              c, "Geometry", FVM_TEXT_FONT_VARIANTS, fvm_test_font_bold_italic, sizeof(fvm_test_font_bold_italic), 0) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void select_style(FVM_Context *c, FVM_Object *format, unsigned variant)
{
    FVM_Value v;
    fvm_value_boolean(&v, (variant & FVM_TEXT_FONT_BOLD) != 0);
    if (!(fvm_object_set(c, format, "bold", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&v, (variant & FVM_TEXT_FONT_ITALIC) != 0);
    if (!(fvm_object_set(c, format, "italic", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif
#ifndef FVM_FONT_FACES_FIXTURE_ONLY


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
    FVM_Object *field = counted_field(c), *format = 0;
    register_styles(c);
    FVM_Value arg, v, out;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 40);
    if (!(fvm_object_set(c, format, "size", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_value_string(&v, "AV") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double widths[] = {48, 56, 52, 60};
    FVM_PathBounds boxes[4];
    for (unsigned variant = 0; variant < 4; ++variant)
    {
        select_style(c, format, variant);
        fvm_value_boolean(&v, 0);
        if (!(fvm_object_set(c, format, "kerning", &v) == FVM_OK &&
              fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK &&
              std::fabs(counted_metric(c, field, "textWidth") - widths[variant]) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Path *path = 0;
        if (!(fvm_textfield_path(c, field, &path) == FVM_OK && fvm_path_bounds(path, boxes + variant) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_path_destroy(path);
        FVM_Value text;
        if (!(fvm_value_string(&text, "AV") == FVM_OK &&
              fvm_object_call(c, format, "getTextExtent", 1, &text, &out) == FVM_OK &&
              std::fabs(counted_metric(c, out.object, "width") - widths[variant]) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, format, "kerning", &v) == FVM_OK &&
              fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK &&
              std::fabs(counted_metric(c, field, "textWidth") - (widths[variant] - 3.2)) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(boxes[1].x_max > boxes[0].x_max && boxes[2].x_max > boxes[0].x_max && boxes[3].x_max > boxes[1].x_max))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
#endif
