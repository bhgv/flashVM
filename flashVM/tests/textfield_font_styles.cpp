/* Real family slots enable bold/italic; missing slots never mutate a field or silently fall back. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"


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
    FVM_Value arg, v, out;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    fvm_value_boolean(&v, 1);
    if (!(fvm_object_set(c, format, "bold", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(c, out.object, "bold", &v) == FVM_OK && v.type == FVM_VALUE_BOOLEAN && !v.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned variant = 1; variant <= FVM_TEXT_FONT_VARIANTS; ++variant)
    {
        if (!(fvm_standard_text_register_font_variant(
                  c, "Geometry", variant, fvm_test_font, sizeof(fvm_test_font), 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned variant = 0; variant <= FVM_TEXT_FONT_VARIANTS; ++variant)
    {
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
        if (!(fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK &&
              fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, out.object, "bold", &v) == FVM_OK && v.type == FVM_VALUE_BOOLEAN &&
              v.boolean == ((variant & FVM_TEXT_FONT_BOLD) != 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, out.object, "italic", &v) == FVM_OK && v.type == FVM_VALUE_BOOLEAN &&
              v.boolean == ((variant & FVM_TEXT_FONT_ITALIC) != 0)))
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
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
