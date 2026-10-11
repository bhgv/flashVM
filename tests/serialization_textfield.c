#include "flashvm/serialization.h"
#include "flashvm/standard_text.h"
#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static FVM_Result missing(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                          FVM_Value *out, void *u)
{
    (void)c;
    (void)s;
    (void)n;
    (void)a;
    (void)u;
    fvm_value_undefined(out);
    return FVM_OK;
}
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *src, *dst;
    FVM_Object *field;
    FVM_Value root, value;
    FVM_Channel *channel;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &src) == FVM_OK);
    assert(fvm_context_create(runtime, &dst) == FVM_OK);
    assert(fvm_standard_text_install(src) == FVM_OK);
    assert(fvm_standard_text_install(dst) == FVM_OK);
#ifndef FVM_HAS_FONTS
    (void)missing;
    (void)root;
    (void)value;
    (void)channel;
    assert(fvm_textfield_create(src, 0, 100, 100, &field) == FVM_UNSUPPORTED);
#else
    {
        FVM_Value copy, args[3], before, after;
        FVM_Object *format, *meta;
        unsigned char *data;
        size_t size;
        unsigned long count_before, count_after, allocations_before, allocations_after;
        FVM_Path *outline;
        assert(fvm_standard_text_register_font(src, "Geometry", fvm_test_font,
                                               sizeof(fvm_test_font), 0) == FVM_OK);
        assert(fvm_textfield_create(src, "Geometry", 81, 200, &field) == FVM_OK);
        assert(fvm_value_string(&value, "AV fi") == FVM_OK);
        assert(fvm_object_set(src, field, "text", &value) == FVM_OK);
        assert(fvm_construct(src, "TextFormat", 0, 0, &format) == FVM_OK);
        fvm_value_number(&value, 40);
        assert(fvm_object_set(src, format, "size", &value) == FVM_OK);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = format;
        assert(fvm_object_call(src, field, "setTextFormat", 1, &value, &after) == FVM_OK);
        fvm_value_number(&value, 12);
        assert(fvm_object_set(src, format, "size", &value) == FVM_OK);
        value.type = FVM_VALUE_OBJECT;
        value.object = format;
        assert(fvm_object_call(src, field, "setNewTextFormat", 1, &value, &after) == FVM_OK);
        assert(fvm_object_get(src, field, "textWidth", &before) == FVM_OK);
        assert(fvm_object_create(src, &meta) == FVM_OK);
        assert(fvm_table_set_metatable(src, field, meta) == FVM_OK);
        fvm_value_undefined(&root);
        root.type = FVM_VALUE_OBJECT;
        root.object = field;
        assert(fvm_object_set(src, field, "self", &root) == FVM_OK);
        assert(fvm_serialization_register_callback(src, "test.missing", missing) == FVM_OK);
        assert(fvm_object_define_method_rooted(src, field, "missing", missing, 0, 0) == FVM_OK);
        assert(fvm_value_serialize(src, &root, &data, &size) == FVM_OK);
        assert(fvm_gc_object_count(dst, &count_before) == FVM_OK);
        assert(fvm_gc_pending_allocations(dst, &allocations_before) == FVM_OK);
        assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_UNSUPPORTED);
        assert(fvm_gc_object_count(dst, &count_after) == FVM_OK && count_before == count_after);
        assert(fvm_gc_pending_allocations(dst, &allocations_after) == FVM_OK &&
               allocations_before == allocations_after);
        /* Font registration also rolled back after a later callback failure. */
        assert(fvm_standard_text_register_font(dst, "Geometry", fvm_test_font,
                                               sizeof(fvm_test_font), 0) == FVM_OK);
        fvm_serialized_free(data);
        assert(fvm_serialization_register_callback(dst, "test.missing", missing) == FVM_OK);
        assert(fvm_channel_open_file("textfield-state.bin", "w", &channel) == FVM_OK);
        assert(fvm_channel_send_value(src, channel, &root, 0) == FVM_OK);
        fvm_channel_destroy(channel);
        fvm_context_destroy(src);
        src = 0;
        /* Recreate a clean receiver: fonts must come entirely from the record. */
        fvm_context_destroy(dst);
        assert(fvm_context_create(runtime, &dst) == FVM_OK);
        assert(fvm_standard_text_install(dst) == FVM_OK);
        assert(fvm_serialization_register_callback(dst, "test.missing", missing) == FVM_OK);
        assert(fvm_channel_open_file("textfield-state.bin", "r", &channel) == FVM_OK);
        assert(fvm_channel_receive_value(dst, channel, &copy, 0) == FVM_OK);
        assert(fvm_gc_pin(dst, copy.object) == FVM_OK);
        assert(fvm_object_get(dst, copy.object, "text", &after) == FVM_OK &&
               !strcmp(after.string, "AV fi"));
        assert(fvm_object_get(dst, copy.object, "textWidth", &after) == FVM_OK &&
               fabs(after.number - before.number) < 1e-8);
        assert(fvm_object_get(dst, copy.object, "self", &after) == FVM_OK &&
               after.object == copy.object);
        assert(fvm_table_get_metatable(dst, copy.object, &meta) == FVM_OK && meta);
        assert(fvm_object_call(dst, copy.object, "getTextFormat", 0, 0, &after) == FVM_OK);
        assert(fvm_object_get(dst, after.object, "size", &value) == FVM_OK && value.number == 40);
        assert(fvm_object_call(dst, copy.object, "getNewTextFormat", 0, 0, &after) == FVM_OK);
        assert(fvm_object_get(dst, after.object, "size", &value) == FVM_OK && value.number == 12);
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 2);
        assert(fvm_value_string(args + 2, "fi") == FVM_OK);
        assert(fvm_object_call(dst, copy.object, "replaceText", 3, args, &after) == FVM_OK);
        assert(fvm_textfield_path(dst, copy.object, &outline) == FVM_OK);
        fvm_path_destroy(outline);
        assert(fvm_gc_collect(dst, &count_after) == FVM_OK);
        assert(fvm_object_get(dst, copy.object, "text", &after) == FVM_OK &&
               !strcmp(after.string, "fi fi"));
        assert(fvm_gc_unpin(dst, copy.object) == FVM_OK);
        fvm_channel_destroy(channel);
        assert(!remove("textfield-state.bin"));
    }
#endif
    if (src)
        fvm_context_destroy(src);
    fvm_context_destroy(dst);
    fvm_runtime_destroy(runtime);
    return 0;
}
