/* Current justification and future styled alignment survive graph/channel owner teardown. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_font_faces.cpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"


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
    FVM_Value arg, v, out, root;
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
    if (!(fvm_value_string(&v, "justify") == FVM_OK && fvm_object_set(c, format, "align", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&v, 1);
    if (!(fvm_object_set(c, field, "multiline", &v) == FVM_OK && fvm_object_set(c, field, "wordWrap", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 94);
    if (!(fvm_object_set(c, field, "_width", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK &&
          fvm_value_string(&v, "A V A V") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK &&
          fvm_value_string(&v, "left") == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    select_style(c, format, FVM_TEXT_FONT_BOLD);
    fvm_value_number(&v, 20);
    if (!(fvm_object_set(c, format, "size", &v) == FVM_OK && fvm_value_string(&v, "right") == FVM_OK &&
          fvm_object_set(c, format, "align", &v) == FVM_OK &&
          fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextFieldState state;
    if (!(fvm_textfield_snapshot(field, state) == FVM_OK && (state.current.alignment & FVM_TEXT_STYLE_JUSTIFY) &&
          !(state.future.alignment & FVM_TEXT_STYLE_JUSTIFY)))
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
    fvm_serialized_free(wire);
    FVM_Object *copy = out.object;
    if (!(fvm_gc_pin(d, copy) == FVM_OK && counted_metric(d, copy, "textWidth") == 90 &&
          counted_metric(d, copy, "_height") == 94))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(d, copy, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(d, out.object, "align", &v) == FVM_OK && !std::strcmp(v.string, "justify")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *ch = 0;
    if (!(fvm_channel_create(&ch) == FVM_OK && fvm_channel_send_value(c, ch, &root, 0) == FVM_OK &&
          fvm_channel_receive_value(d, ch, &out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    if (!(counted_metric(d, out.object, "textWidth") == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(fvm_gc_collect(d, 0) == FVM_OK && fvm_value_string(&v, "A V A V") == FVM_OK &&
          fvm_object_set(d, copy, "text", &v) == FVM_OK && counted_metric(d, copy, "textWidth") == 74 &&
          counted_metric(d, copy, "_height") == 26))
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
