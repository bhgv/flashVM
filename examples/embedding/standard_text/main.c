#include "flashvm/standard_text.h"
#include "../../../tests/fixtures/font_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *format = 0, *field = 0;
    FVM_Value value, args[3], result;
    FVM_Path *outline = 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_standard_text_install(context) == FVM_OK);
#ifdef FVM_HAS_FONTS
    assert(fvm_standard_text_register_font(context, "Geometry", fvm_test_font,
                                           sizeof(fvm_test_font), 0) == FVM_OK);
    assert(fvm_construct(context, "TextFormat", 0, 0, &format) == FVM_OK);
    fvm_value_number(&value, 40);
    assert(fvm_object_set(context, format, "size", &value) == FVM_OK);
    assert(fvm_textfield_create(context, "Geometry", 61, 140, &field) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = field;
    assert(fvm_set_global(context, "caption", &value) == FVM_OK);
    fvm_value_boolean(&value, 1);
    assert(fvm_object_set(context, field, "multiline", &value) == FVM_OK);
    assert(fvm_object_set(context, field, "wordWrap", &value) == FVM_OK);
    fvm_value_undefined(&args[0]);
    args[0].type = FVM_VALUE_OBJECT;
    args[0].object = format;
    assert(fvm_object_call(context, field, "setNewTextFormat", 1, args, &result) == FVM_OK);
    assert(fvm_value_string(&value, "AV fi") == FVM_OK);
    assert(fvm_object_set(context, field, "text", &value) == FVM_OK);
    assert(fvm_object_get(context, field, "textHeight", &result) == FVM_OK);
    printf("caption.textHeight = %.2f pixels\n", result.number);
    /* Replace [0,2), retaining the current uniform style and invalidating layout. */
    fvm_value_number(&args[0], 0);
    fvm_value_number(&args[1], 2);
    assert(fvm_value_string(&args[2], "B") == FVM_OK);
    assert(fvm_object_call(context, field, "replaceText", 3, args, &result) == FVM_OK);
    assert(fvm_object_get(context, field, "text", &result) == FVM_OK);
    printf("caption.text = %s\n", result.string);
    assert(strcmp(result.string, "B fi") == 0);
    fvm_value_number(&args[0], 0);
    assert(fvm_object_call(context, field, "getTextFormat", 1, args, &result) == FVM_OK);
    assert(fvm_object_get(context, result.object, "size", &value) == FVM_OK);
    assert(value.type == FVM_VALUE_NUMBER && value.number == 40);
    assert(fvm_textfield_path(context, field, &outline) == FVM_OK);
#else
    (void)format;
    (void)field;
    (void)value;
    (void)args;
    (void)result;
    puts("TextFormat objects are available; font measurement requires FONTS.");
#endif
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    /* Outlines are owned independently and can be edited/drawn/masked here. */
    fvm_path_destroy(outline);
    return 0;
}
