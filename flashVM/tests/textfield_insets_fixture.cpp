/* Positional TextFormat construction supplies independent paragraph inset values. */
#define FVM_INSETS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_insets_fixture.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *format = 0;
    FVM_Value args[13], out;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c);
    for (unsigned i = 0; i < 13; ++i)
    {
        fvm_value_undefined(args + i);
        args[i].type = FVM_VALUE_NULL;
    }
    fvm_value_number(args + 1, 40);
    fvm_value_number(args + 9, 10);
    fvm_value_number(args + 10, 12);
    fvm_value_number(args + 11, 0);
    fvm_value_number(args + 12, 2);
    if (!(fvm_construct(c, "TextFormat", 13, args, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "AV");
    inset_string(c, field, "autoSize", "left");
    if (!(counted_metric(c, field, "_width") == 74 && counted_metric(c, field, "textWidth") == 48))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
          counted_metric(c, out.object, "leftMargin") == 10 && counted_metric(c, out.object, "rightMargin") == 12 &&
          counted_metric(c, out.object, "blockIndent") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
