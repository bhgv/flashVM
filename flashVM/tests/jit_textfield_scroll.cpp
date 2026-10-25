/* Scroll SetMember/GetMember agree in interpreter/manual/native JIT with a GC barrier during conversion. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_fixture.hpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#ifdef FVM_HAS_FONTS
static FVM_Result
widen_on_conversion(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    scroll_number(c, (FVM_Object *)raw, "_width", 100);
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 7);
    return FVM_OK;
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *field = scroll_field(c);
            FVM_Value v = class_value(field);
            if (!(fvm_set_global(c, "field", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program p;
            p.variable("field");
            p.text("scroll");
            p.number(2.9);
            p.op(0x4f);
            p.variable("field");
            p.text("hscroll");
            p.number(7.9);
            p.op(0x4f);
            p.variable("field");
            p.text("bottomScroll");
            p.op(0x4e);
            p.finish();
            v = class_run(c, p);
            if (!(v.type == FVM_VALUE_NUMBER && v.number == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_TextFieldScrollInfo info = scroll_info(c, field);
            if (!(info.scroll == 2 && info.hscroll == 7))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *request = 0;
            if (!(fvm_object_create(c, &request) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_NativeEntry entry;
            entry.callback = widen_on_conversion;
            entry.userdata = field;
            entry.root = field;
            request->methods["valueOf"] = entry;
            v = class_value(request);
            if (!(fvm_set_global(c, "request", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program reentry;
            reentry.variable("field");
            reentry.text("hscroll");
            reentry.variable("request");
            reentry.op(0x4f);
            reentry.variable("field");
            reentry.text("hscroll");
            reentry.op(0x4e);
            reentry.finish();
            v = class_run(c, reentry);
            if (!(v.type == FVM_VALUE_NUMBER && v.number == 0 && counted_metric(c, field, "_width") == 100 &&
                  counted_metric(c, field, "maxhscroll") == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
