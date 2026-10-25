/* Selected current/future faces cross binary/channel boundaries without regular-face substitution. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_font_faces.cpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *d = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c), *format = 0;
    register_styles(c);
    FVM_Value v, arg, out, root;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
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
    select_style(c, format, FVM_TEXT_FONT_BOLD);
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK &&
          fvm_value_string(&v, "AV") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&v, "center") == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK &&
          counted_metric(c, field, "_width") == 60 && counted_metric(c, field, "_x") == 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    select_style(c, format, FVM_TEXT_FONT_ITALIC);
    fvm_value_number(&v, 20);
    if (!(fvm_object_set(c, format, "size", &v) == FVM_OK &&
          fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = field;
    unsigned char *wire = 0;
    size_t size = 0;
    if (!(fvm_standard_text_install(d) == FVM_OK && fvm_value_serialize(c, &root, &wire, &size) == FVM_OK &&
          fvm_value_deserialize(d, wire, size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *copy = out.object;
    if (!(fvm_gc_pin(d, copy) == FVM_OK && d->fonts.size() == 2 && counted_metric(d, copy, "textWidth") == 56))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(d, copy, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(d, out.object, "bold", &v) == FVM_OK && v.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(d, copy, "getNewTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(d, out.object, "italic", &v) == FVM_OK && v.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    FVM_Channel *ch = 0;
    if (!(fvm_channel_create(&ch) == FVM_OK && fvm_channel_send_value(c, ch, &root, 0) == FVM_OK &&
          fvm_channel_receive_value(d, ch, &out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    if (!(counted_metric(d, out.object, "textWidth") == 56))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(fvm_gc_collect(d, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&v, "AVA") == FVM_OK && fvm_object_set(d, copy, "text", &v) == FVM_OK &&
          counted_metric(d, copy, "textWidth") == 39 && counted_metric(d, copy, "_width") == 43 &&
          counted_metric(d, copy, "_x") == 28.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *plain = 0;
    if (!(fvm_construct(d, "TextFormat", 0, 0, &plain) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    select_style(d, plain, FVM_TEXT_FONT_REGULAR);
    arg.object = plain;
    if (!(fvm_object_call(d, copy, "setTextFormat", 1, &arg, &out) == FVM_NOT_FOUND &&
          counted_metric(d, copy, "textWidth") == 39))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *path = 0;
    if (!(fvm_textfield_path(d, copy, &path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(d);
    if (!(fvm_path_count(path) > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
