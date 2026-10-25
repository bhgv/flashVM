/* Actual AVM1 anchor and signed-position writes agree in interpreter and both native JIT modes. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = counted_field(c), *format = 0;
        FVM_Value value, out, arg;
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
        arg = class_value(format);
        if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(field);
        if (!(fvm_set_global(c, "field", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&value, "AV") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned anchor = 0; anchor < 3; ++anchor)
        {
            fvm_value_boolean(&value, 0);
            if (!(fvm_object_set(c, field, "autoSize", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 100);
            if (!(fvm_object_set(c, field, "_width", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 10);
            if (!(fvm_object_set(c, field, "_x", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_value_string(&value, "AV") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program p;
            p.variable("field");
            p.text("autoSize");
            p.text(anchor == 0 ? "left" : anchor == 1 ? "center" : "right");
            p.op(0x4f);
            p.variable("field");
            p.text("text");
            p.text("AVA");
            p.op(0x4f);
            p.variable("field");
            p.text("_x");
            p.op(0x4e);
            p.finish();
            out = class_run(c, p);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 10 + 24 * anchor * 0.5))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(counted_metric(c, field, "_width") == 76 && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames &&
                  !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program move;
            move.variable("field");
            move.text("_y");
            move.number(-33);
            move.op(0x4f);
            move.variable("field");
            move.text("_y");
            move.op(0x4e);
            move.finish();
            out = class_run(c, move);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == -33))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
