/* Legacy built-in stubs upgrade; forged automatic dimensions never attach native state. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"


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
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&value, "AV") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_NativeFunction retired = fvm_text_callback_from_id(205);
    FVM_AccessorEntry &legacy = field->accessors["autoSize"];
    legacy.getter = legacy.setter = retired;
    legacy.userdata = 0;
    legacy.root = 0;
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = field;
    unsigned char *wire = 0;
    size_t size = 0;
    if (!(fvm_standard_text_install(other) == FVM_OK && fvm_value_serialize(c, &root, &wire, &size) == FVM_OK &&
          fvm_value_deserialize(other, wire, size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    FVM_Object *copy = out.object;
    if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_object_get(other, copy, "autoSize", &value) == FVM_OK &&
          !std::strcmp(value.string, "none")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&value, 1);
    if (!(fvm_object_set(other, copy, "autoSize", &value) == FVM_OK && counted_metric(other, copy, "_width") == 52))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextFieldState state;
    if (!(fvm_textfield_snapshot(copy, state) == FVM_OK && state.auto_size_left))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned which = 0; which < 2; ++which)
    {
        FVM_TextFieldState bad = state;
        FVM_Object *object = 0;
        if (which)
        {
            bad.height += 1;
        }
        else
        {
            bad.width += 1;
        }
        if (!(fvm_object_create(other, &object) == FVM_OK &&
              fvm_textfield_restore(other, object, bad) == FVM_INVALID_ARGUMENT && !object->resource))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Host captures must not be replaced, even with the retired callback identities. */
    FVM_AccessorEntry captured;
    captured.getter = captured.setter = retired;
    captured.root = copy;
    fvm_textfield_upgrade_accessor("autoSize", captured);
    if (!(captured.getter == retired && captured.root == copy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    captured.root = 0;
    captured.userdata = &state;
    fvm_textfield_upgrade_accessor("autoSize", captured);
    if (!(captured.getter == retired && captured.userdata == &state))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(fvm_gc_collect(other, 0) == FVM_OK && counted_metric(other, copy, "_width") == 52))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
