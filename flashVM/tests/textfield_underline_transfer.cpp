/* Current/future underline bits survive binary/channel transfer without increasing wire size. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c), *format = 0;
    FVM_Value arg, value, out, root;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 40);
    if (!(fvm_object_set(c, format, "size", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&value, 1);
    if (!(fvm_object_set(c, format, "underline", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_value_string(&value, "AV") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK &&
          fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextFieldState state;
    if (!(fvm_textfield_snapshot(field, state) == FVM_OK && (state.current.alignment & FVM_TEXT_STYLE_UNDERLINE) &&
          !(state.future.alignment & FVM_TEXT_STYLE_UNDERLINE)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *path = 0;
    if (!(fvm_textfield_path(c, field, &path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t decorated = fvm_path_count(path);
    fvm_path_destroy(path);
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = field;
    if (!(fvm_standard_text_install(other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char *wire = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK &&
          fvm_value_deserialize(other, wire, size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    FVM_Object *copy = out.object;
    if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_textfield_path(other, copy, &path) == FVM_OK &&
          fvm_path_count(path) == decorated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    if (!(fvm_object_call(other, copy, "getNewTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(other, out.object, "underline", &value) == FVM_OK && value.type == FVM_VALUE_BOOLEAN &&
          !value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *channel = 0;
    if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &root, 0) == FVM_OK &&
          fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(channel);
    if (!(fvm_textfield_path(other, out.object, &path) == FVM_OK && fvm_path_count(path) == decorated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    fvm_value_boolean(&value, 0);
    if (!(fvm_object_set(c, format, "underline", &value) == FVM_OK &&
          fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t plain_size = 0;
    if (!(fvm_value_serialize(c, &root, &wire, &plain_size) == FVM_OK && plain_size == size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    /* A separate round trip carries the opposite current/future selection. */
    fvm_value_boolean(&value, 1);
    if (!(fvm_object_set(c, format, "underline", &value) == FVM_OK &&
          fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_serialize(c, &root, &wire, &plain_size) == FVM_OK &&
          fvm_value_deserialize(other, wire, plain_size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    FVM_Object *future_copy = out.object;
    if (!(fvm_gc_pin(other, future_copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(other, future_copy, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(other, out.object, "underline", &value) == FVM_OK && !value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(other, future_copy, "getNewTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(other, out.object, "underline", &value) == FVM_OK && value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&value, "AVA") == FVM_OK && fvm_object_set(other, future_copy, "text", &value) == FVM_OK &&
          fvm_object_call(other, future_copy, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(other, out.object, "underline", &value) == FVM_OK && value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Legacy record has no underline bit and restores undecorated current geometry. */
    state.current.alignment &= ~FVM_TEXT_STYLE_UNDERLINE;
    FVM_Object *legacy = 0;
    if (!(fvm_object_create(other, &legacy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    legacy->resource_kind = copy->resource_kind;
    legacy->accessors = copy->accessors;
    legacy->methods = copy->methods;
    if (!(fvm_textfield_restore(other, legacy, state) == FVM_OK && fvm_textfield_path(other, legacy, &path) == FVM_OK &&
          fvm_path_count(path) + 5 == decorated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    fvm_context_destroy(c);
    if (!(fvm_gc_collect(other, 0) == FVM_OK && fvm_textfield_path(other, copy, &path) == FVM_OK &&
          fvm_path_count(path) == decorated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    if (!(fvm_value_string(&value, "AVA") == FVM_OK && fvm_object_set(other, copy, "text", &value) == FVM_OK &&
          fvm_object_call(other, copy, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(other, out.object, "underline", &value) == FVM_OK && !value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
