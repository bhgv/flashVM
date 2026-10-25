/* Actual AVM1 SetMember/CallMethod/GetMember select the same kerning in interpreter and native JIT. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include <cmath>


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
        FVM_Value value, out;
        if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
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
        value = class_value(format);
        if (!(fvm_set_global(c, "format", &value) == FVM_OK))
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
        if (!(fvm_value_string(&value, "AV") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned enabled = 0; enabled < 2; ++enabled)
        {
            Program p;
            p.variable("format");
            p.text("kerning");
            p.number(enabled);
            p.op(0x4f);
            p.variable("format");
            p.number(1);
            p.variable("field");
            p.text("setTextFormat");
            p.op(0x52);
            p.op(0x17);
            p.variable("field");
            p.text("textWidth");
            p.op(0x4e);
            p.finish();
            out = class_run(c, p);
            if (!(out.type == FVM_VALUE_NUMBER && std::fabs(out.number - (enabled ? 44.8 : 48)) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
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
