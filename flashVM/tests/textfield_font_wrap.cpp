/* Real face advances select wrapping and automatic height; glyph/underline paths retain ownership. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_font_faces.cpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned variant = 0; variant < 4; ++variant)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = counted_field(c), *format = 0;
        register_styles(c);
        FVM_Value v, arg, out;
        if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        select_style(c, format, variant);
        fvm_value_number(&v, 40);
        if (!(fvm_object_set(c, format, "size", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, format, "underline", &v) == FVM_OK &&
              fvm_object_set(c, field, "multiline", &v) == FVM_OK &&
              fvm_object_set(c, field, "wordWrap", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 60);
        if (!(fvm_object_set(c, field, "_width", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = format;
        if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "AVAV") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK &&
              fvm_value_string(&v, "right") == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(counted_metric(c, field, "_height") == (variant == 3 ? 186 : 94) && counted_metric(c, field, "_x") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 64);
        if (!(fvm_object_set(c, field, "_width", &v) == FVM_OK && counted_metric(c, field, "_height") == 94))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Path *path = 0;
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_textfield_path(c, field, &path) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        if (!(fvm_path_count(path) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_path_destroy(path);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
