/* Paragraph insets reduce wrapping width while preserving glyph-only measurements. */
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
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *names[] = {"size", "leftMargin", "rightMargin", "blockIndent"};
    const double values[] = {40, 10, 12, 8};
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_value_number(&v, values[i]);
        if (!(fvm_object_set(c, format, names[i], &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_boolean(&v, 1);
    if (!(fvm_object_set(c, field, "multiline", &v) == FVM_OK && fvm_object_set(c, field, "wordWrap", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 124);
    if (!(fvm_object_set(c, field, "_width", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK &&
          fvm_value_string(&v, "A V A V") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(counted_metric(c, field, "textWidth") == 72 && counted_metric(c, field, "textHeight") == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&v, "left") == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK &&
          counted_metric(c, field, "_height") == 94))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 1; i < 4; ++i)
    {
        if (!(counted_metric(c, out.object, names[i]) == values[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Path *path = 0;
    if (!(fvm_textfield_path(c, field, &path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_PathBounds bounds;
    if (!(fvm_path_bounds(path, &bounds) == FVM_OK && bounds.x_min >= 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    fvm_value_boolean(&v, 0);
    if (!(fvm_object_set(c, field, "wordWrap", &v) == FVM_OK && counted_metric(c, field, "_width") == 166 &&
          counted_metric(c, field, "textWidth") == 132))
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
